# ThermometerBT_stm32_HAL
Application on nucleo stm32f401re doing a simple temperature measurement using HAL library.

On the final version i want to do temperature measurment that can be triggered by bluetooth. Measurment will be send to external bluetooth device
or displayed on 4x8segment display. After x minutes/measurment device will enter sleep mode and wait for next uart bluetooth interrupt.

Bluetooth module : JDY-09 (UART)  
Temparature sensor : TMP102 (I2C)  
4x8 segment display : TM1637 (GPIO)

Version 03.12.2021 :

Pins connected:
JDY09 :  
PA9 - USART1_RX (connection between MCU and JDY-09)  
PA10 - USART1_TX (connection between MCU and JDY-09)  
PC3 - GPIO_EXTI (JDY-09 State pin)  
PA3 - USART2_RX (connection between MCU and PC)  
PA2 - USART2_TX (connection between MCU and PC)

TMP102 :  
PB6 - I2C_SCL  
PB7 - I2C_SDA  
PB9 - TMP102_ALERT (GPIO INPUT PP)  

TM1637:  
PC1 - TM1637_CLK (GPIO OTUPUT OD PU)    
PC2 - TM1637_DIO (GPIO OUTPUT OD PU)  

Recieved messages trigger IRQ from usart/dma. Main function checks if there is line recieved (messages finished with +CR +LF).  
If message is finished with semicolon (example : MEASURE;) then parser recognize it as command and do certain action.  
  
Module libs used:  
JDY-09 - my lib (on github)  
TMP102 - my lib (on github)  
TM1637 - https://github.com/rogerdahl/stm32-tm1637  
