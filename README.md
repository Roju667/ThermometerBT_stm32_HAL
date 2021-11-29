# jdy_09_bluetooth
Application on nucleo stm32f401re using JDY-09 module with my library.

On the final version i want to do temperature measurment that can be triggered by bluetooth. Measurment will be send to external bluetooth device
or displayed on 4x8segment display. After x minutes/measurment device will enter sleep mode and wait for next uart bluetooth interrupt.

Bluetooth module : JDY-09 (UART)
Temparature sensor : TMP102 (I2C)
4x8 segment display : GPIO

Version 29.11.2021 :

Pins connected:
PA9 - USART1_RX (connection between MCU and JDY-09)
PA10 - USART1_TX (connection between MCU and JDY-09)
PC3 - GPIO_EXTI (JDY-09 State pin)
PA3 - USART2_RX (connection between MCU and PC)
PA2 - USART2_TX (connection between MCU and PC)

Recieved messages trigger IRQ from usart/dma. Main function checks if there is line recieved (messages finished with +CR +LF).
If message is finished with semicolon (example : MEASURE;) then parser recognize it as command and do certain action.
