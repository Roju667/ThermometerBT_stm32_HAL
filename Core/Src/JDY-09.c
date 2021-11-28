/*
 * JDY-09.c
 *
 *  Created on: Nov 24, 2021
 *      Author: ROJEK
 */

/* CubeMX default settings for UART :
 * Baud: 9600
 * Word lenght : 8
 * Parity : None
 * Stop Bits : 1
 *
 * To receive message by this lib - global IRQ for UART has to be enabled.
 * Put JDY09_CallbackRecieve in HAL_UART_RxCpltCallback to get data to ring buffer of BT device.
 * In main function use JDY09_CallbackRecieve to check if there is message pending in Ring buffer.
 *
 * To write AT+Commands device has to be disconnected from master. Connection is defined by GPIO pin STATE.
 *
 * Use EXTI IRQ to get a message whenever device is connected/disconnected
 */

#include "main.h"
#include "usart.h"
#include "ringbuffer.h"
#include "JDY-09.h"
#include "stdio.h"
#include "string.h"

// Messages for display terminal

/*
 * Terminal defined by user, commands sent in offline mode will be displayed
 * with responses from JDY-09
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[Command] - predefined command to send
 * @return - void
 */

void JDY09_DisplayTerminal(char *Msg)
{
	uint8_t Lenght = strlen(Msg);
	HAL_UART_Transmit(&huart2, (uint8_t*) Msg, Lenght, JDY09_UART_TIMEOUET);
}

/*
 * Simple command send between MCU and JDY-09
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[Command] - predefined command to send
 * @return - void
 */
static void SendAndDisplayCmd(JDY09_t *jdy09, uint8_t *Command)
{
	uint8_t MsgRecieved[64];

	//display send info on user display terminal
	JDY09_DisplayTerminal("Sending: ");
	JDY09_DisplayTerminal((char*) Command);

	//send data to JDY-09
	HAL_UART_Transmit(jdy09->huart, Command, strlen((char*) Command), JDY09_UART_TIMEOUET);

	//wait for response line
	while (jdy09->LinesRecieved == 0)
	{
	}

	//get message out of ring buffer
	JDY09_CheckPendingMessages(jdy09, MsgRecieved);

	//display response
	JDY09_DisplayTerminal("Response: ");
	JDY09_DisplayTerminal((char*) MsgRecieved);

	//clear message pending flag
	JDY09_ClearMsgPendingFlag(jdy09);
}

/*
 * Get baud rate
 *
 * @param[Baudrate] - predefined baud rate 4-9
 *
 * @return - baud rate value 9600 - 128000;
 */
static uint32_t JDY09_GetBaud(uint8_t Baudrate)
{
	switch (Baudrate)
	{
	case JDY09_BAUDRATE_9600:
		return 9600;

	case JDY09_BAUDRATE_19200:
		return 19200;

	case JDY09_BAUDRATE_38400:
		return 38400;

	case JDY09_BAUDRATE_57600:
		return 57600;

	case JDY09_BAUDRATE_115200:
		return 115200;

	case JDY09_BAUDRATE_128000:
		return 128000;

	}

	return 9600;
}
/*
 * Initialize JDY-09 structure
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[huart] - handler to uart that JDY-09 is connected to
 * @param[StateGPIOPort] - handle to GPIO port of STATE pin
 * @param[StateGPIOPin] - pin number of STATE pin
 *
 * @return - void
 */
void JDY09_Init(JDY09_t *jdy09, UART_HandleTypeDef *huart, GPIO_TypeDef *StateGPIOPort, uint16_t StateGPIOPin)
{

	// after trials i found out that module will not communicate correctly after loading new program and starting it right away
	// 6ms was minimum that worked everytime after loading some changes

	HAL_Delay(100);
	// Assign uart
	jdy09->huart = huart;

	// Assign GPIO for State pin
	jdy09->StateGPIOPort = StateGPIOPort;
	jdy09->StatePinNumber = StateGPIOPin;

	HAL_UART_Receive_IT(jdy09->huart, &(jdy09->TmpBufferBT), 1);

	//during init - disconnect and display basic information
	JDY09_Disconnect(jdy09);

	JDY09_SendCommand(jdy09, JDY09_CMD_GETVERSION);
	JDY09_SendCommand(jdy09, JDY09_CMD_GETADRESS);
	JDY09_SendCommand(jdy09, JDY09_CMD_GETBAUDRATE);
	JDY09_SendCommand(jdy09, JDY09_CMD_GETNAME);
	JDY09_SendCommand(jdy09, JDY09_CMD_GETPASSWORD);
}

/*
 * Send selected command to JDy-09 in offline mode
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[Command] - predefined commands that are in .h file
 *
 * @return - void
 */
void JDY09_SendCommand(JDY09_t *jdy09, JDY09_CMD Command)
{
	// check if there is no connection
	if (HAL_GPIO_ReadPin(jdy09->StateGPIOPort, jdy09->StatePinNumber) == GPIO_PIN_RESET)
	{
		switch (Command)
		{
		case JDY09_CMD_GETVERSION:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+VERSION\r\n");
			break;

		case JDY09_CMD_RESET:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+RESET\r\n");
			break;

		case JDY09_CMD_GETADRESS:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+LADDR\r\n");
			break;

		case JDY09_CMD_GETBAUDRATE:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+BAUD\r\n");
			break;

		case JDY09_CMD_GETPASSWORD:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+PIN\r\n");
			break;

		case JDY09_CMD_GETNAME:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+NAME\r\n");
			break;

		case JDY09_CMD_SETDEFAULTSETTINGS:
			SendAndDisplayCmd(jdy09, (uint8_t*) "AT+DEFAULT\r\n");
			break;
		}
		return;
	}

	// AT cmd error
	JDY09_DisplayTerminal("AT commands possible only in offline mode \n\r");

}

/*
 * Disconnect from BT device using MCU
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @return - void
 */
void JDY09_Disconnect(JDY09_t *jdy09)
{
	//check connection
	if (HAL_GPIO_ReadPin(jdy09->StateGPIOPort, jdy09->StatePinNumber) == GPIO_PIN_SET)
	{
		// disconnect
		SendAndDisplayCmd(jdy09, (uint8_t*) "AT+DISC\r\n");
		return;
	}

	// AT cmd error
	JDY09_DisplayTerminal("Module already disconnected \n\r");
}

/*
 * Set new baud rate from MCU level
 * function is problematic with hal initialization of uart
 * user has to remember to change it in CubeMX
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[Baudrate] - predefined baud rate
 * @return - void
 */
void JDY09_SetBaudRate(JDY09_t *jdy09, uint8_t Baudrate)
{
	//check if there is no connection
	if (HAL_GPIO_ReadPin(jdy09->StateGPIOPort, jdy09->StatePinNumber) == GPIO_PIN_RESET)
	{

		//send new baudrate
		uint8_t Msg[16];
		sprintf((char*) Msg, "AT+BAUD%d\r\n", Baudrate);
		SendAndDisplayCmd(jdy09, Msg);
		JDY09_DisplayTerminal("New baud set - restart device \n\r");

		return;

	}

	// AT cmd error
	JDY09_DisplayTerminal("AT commands possible only in offline mode \n\r");
}

/*
 * Set new name from MCU level
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[*Name] - pointer to array with new password
 * @return - void
 */
void JDY09_SetName(JDY09_t *jdy09, uint8_t *Name)
{
	// check if name is not too long
	if (strlen((char*) Name) > JDY09_MAX_NAME_LENGHT)
	{
		JDY09_DisplayTerminal("Defined name too long, max 16 chars");
		return;
	}

	// check if there is no active connection
	if (HAL_GPIO_ReadPin(jdy09->StateGPIOPort, jdy09->StatePinNumber) == GPIO_PIN_RESET)
	{
		uint8_t Msg[32];
		sprintf((char*) Msg, "AT+NAME%s\r\n", Name);
		SendAndDisplayCmd(jdy09, Msg);
		JDY09_DisplayTerminal("New name set - restart device \n\r");

		return;
	}

	// AT cmd error
	JDY09_DisplayTerminal("AT commands possible only in offline mode \n\r");
}

/*
 * Set password from MCU level
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[Password] - new password
 * @return - void
 */
void JDY09_SetPassword(JDY09_t *jdy09, uint8_t *Password)
{
	// check if pin is not too long
	if (strlen((char*) Password) > JDY09_MAX_PIN_LENGHT)
	{
		JDY09_DisplayTerminal("Defined pin too long, max 4 digits");
		return;
	}

	// check if there is no active connection
	if (HAL_GPIO_ReadPin(jdy09->StateGPIOPort, jdy09->StatePinNumber) == GPIO_PIN_RESET)
	{
		uint8_t Msg[32];
		sprintf((char*) Msg, "AT+PIN%s\r\n", Password);
		SendAndDisplayCmd(jdy09, Msg);
		JDY09_DisplayTerminal("New pin set - restart device \n\r");

		return;
	}

	// AT cmd error
	JDY09_DisplayTerminal("AT commands possible only in offline mode \n\r");
}

/*
 * Clear message pending flag
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @return - void
 */
void JDY09_ClearMsgPendingFlag(JDY09_t *jdy09)
{
	jdy09->MessagePending = JDY09_NOMESSAGE;
}

/*
 * Check if there is a message Pending, if yes -> write it to a MsgBuffer
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[*MsgBuffer] - pointer to buffer where message has to be written
 * @return - status : massage pending 1/0
 */
uint8_t JDY09_CheckPendingMessages(JDY09_t *jdy09, uint8_t *MsgBuffer)
{

	// Check if there is message finished
	if (jdy09->LinesRecieved > 0)
	{

		uint8_t i = 0;
		uint8_t temp = 0;
		do
		{
			// Move a sign to ring buffer
			RB_Read(&(jdy09->RingBuffer), &temp);
			if (temp == JDY09_LASTCHARACTER)
			{
				MsgBuffer[i] = JDY09_LASTCHARACTER;
				MsgBuffer[i + 1] = 0;
			}
			else
			{
				MsgBuffer[i] = temp;
			}
			i++;
			//rewrite signs until last character defined by user
		} while (temp != JDY09_LASTCHARACTER);
		//decrement LinesRecieved
		jdy09->LinesRecieved--;
		//set up flag that message is ready to parse
		jdy09->MessagePending = JDY09_MESSAGEPENDING;
	}

	// return if flag status
	return jdy09->MessagePending;
}

/*
 * Callback to put in HAL_UART_RxCpltCallback
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[*huart] - uart handle
 * @return - void
 */
void JDY09_RxCpltCallback(JDY09_t *jdy09, UART_HandleTypeDef *huart)
{

	//check if IRQ is coming from correct uart
	if (jdy09->huart->Instance == huart->Instance)
	{
		//write a sign to ring buffer
		RB_Write((&(jdy09->RingBuffer)), jdy09->TmpBufferBT);

		// when line is complete -> add 1 to received lines
		if (jdy09->TmpBufferBT == JDY09_LASTCHARACTER)
		{
			(jdy09->LinesRecieved)++;
		}

		// start another IRQ for single sign
		HAL_UART_Receive_IT(jdy09->huart, &(jdy09->TmpBufferBT), 1);
	}
}

/*
 * Callback to put in HAL_GPIO_EXTI_Callback
 *
 * @param[*jdy09] - pointer to struct for JDY09 bluetooth module
 * @param[GPIO_Pin] - pin number from EXTI
 * @return - void
 */
void JDY09_EXTICallback(JDY09_t *jdy09, uint16_t GPIO_Pin)
{

	//check if IRQ is coming from STATE pin
	if (jdy09->StatePinNumber == GPIO_Pin)
	{
		// if trigger is caused by rising edge then new connection is made
		if (HAL_GPIO_ReadPin(BT_STATE_GPIO_Port, BT_STATE_Pin) == GPIO_PIN_SET)
		{
			JDY09_DisplayTerminal("Device connected \n\r");
		}
		else
		// if trigger is from falling edge then msg disconnect
		{
			JDY09_DisplayTerminal("Device disconnected \n\r");
		}
	}
}