/*
 * parse.c
 *
 *  Created on: 24 cze 2021
 *      Author: ROJEK
 */

#include "main.h"
#include "string.h"
#include "ringbuffer.h"
#include "parse.h"
#include "usart.h"

void Parser_DisplayTerminal(char *Msg)
{
	uint8_t Lenght = strlen(Msg);
	HAL_UART_Transmit(&huart2, (uint8_t*) Msg, Lenght, 1000);
}


void Parse_WriteDataToBuffer(Ringbuffer_t *RecieveBuffer, uint8_t *ParseBuffer)
{
	uint8_t i = 0;
	uint8_t temp;
	do
	{
		RB_Read(RecieveBuffer, &temp);
		if (temp == ENDLINE)
		{
			ParseBuffer[i] = '\n';
			ParseBuffer[i + 1] = 0;
		}
		else
		{
			ParseBuffer[i] = temp;
		}
		i++;
	} while (temp != ENDLINE);
}

/*
 * @ WAKE UP procedure
 */
static void Parser_WAKE_UP(void)
{
	//wake up for 5 mins
	Parser_DisplayTerminal("System wake up\n\r");

}

/*
 * @ MEASURE procedure
 */
static void Parser_MEASURE(void)
{
	// send log to uart
	Parser_DisplayTerminal("Calculating HUGE data\n\r");

	//bmp280 measure

	//bluetooth send to master
}

/*
 * @ DISPLAY procedure
 */
static void Parser_DISPLAY(void)
{
	// send log to uart
	Parser_DisplayTerminal("Parametres displayed \n\r");

	//start timer to measure every 1 second for 1 minute
}

/*
 * @ SLEEP procedure
 */
static void Parser_SLEEP(uint8_t *ParseBuffer)
{
//	//execute sleep
//
//	//stop timer
//	HAL_TIM_Base_Stop_IT(&htim1);
//
//	//delete all the commands after SLEEP
//	memset(ParseBuffer,0,(sizeof(ParseBuffer)));
//
//	//send log on uart
//	UartLogBT("Im going to sleep comadre, cya\n\r");
//
//	//stop hal tick
//	HAL_SuspendTick();
//
//	//enter sleep mode -> it will wait for IRQ to wake up
//	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
//
//	//after wake up continue tick
//	HAL_ResumeTick();
//
//	//send log on uart
//	UartLogBT("Welcome back broski\n\r");
//
//	//start count down for going back to sleep
//	HAL_TIM_Base_Start_IT(&htim1);

}


/*
 * @ function parse message and start command procedures
 */

uint8_t Parser_Parse(uint8_t *ParseBuffer)
{
	// Count how many commands we have to parse
	uint8_t cmd_count = 0;
	uint8_t i = 0;
	uint8_t LastCommand[16] =  {0};

	// For every semicolon count up until EOL
	do
	{
		if (ParseBuffer[i] == ';')
		{
			cmd_count++;
		}
		i++;
	} while (ParseBuffer[i] != '\n');

	// no command was found
	if (cmd_count == 0)
	{
		Parser_DisplayTerminal("Command unknown : \n\r");
		Parser_DisplayTerminal("Send HELP; to get commands\n\r");
		// return ERROR
		return PARSE_ERROR_NOCMD;
	}


	uint8_t *ParsePointer;

	// Execute cmd_count number of commands
	for (i = 0; i < cmd_count; i++)
	{

		// cut command from the message -> from beginning to ;
		//if first msg start from beginning
		if(i == 0)
		{
			ParsePointer = (uint8_t*)(strtok((char*)ParseBuffer, ";"));

		}else
		{
			ParsePointer = (uint8_t*)(strtok(NULL, ";"));
		}


		// if you put two same commands in a row - error
		if(strcmp((char*)ParsePointer,(char*)LastCommand) == 0)
		{
			Parser_DisplayTerminal("Error, same command twice in a row!\n\r");
			return PARSE_ERROR_2CMDS;
		}

		/*
		 * EXECUTE COMMANDS
		 */

		// do WAKE_UP
		if (strcmp("WAKE_UP", (char*)ParsePointer) == 0)
		{
			Parser_WAKE_UP();
		}
		// do MEASURE
		else if (strcmp("MEASURE", (char*)ParsePointer) == 0)
		{
			Parser_MEASURE();
		}
		// do DISPLAY
		else if (strcmp("DISPLAY", (char*)ParsePointer) == 0)
		{
			Parser_DISPLAY();
		}
		// do SLEEP
		else if (strcmp("SLEEP", (char*)ParsePointer) == 0)
		{
			Parser_SLEEP(ParsePointer);
			return PARSE_OK;
		}

		strcpy((char*)LastCommand,(char*)ParsePointer);
	}

	return PARSE_OK;
}
