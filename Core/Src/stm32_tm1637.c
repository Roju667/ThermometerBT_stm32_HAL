#include "stm32f4xx_hal.h"

#include "stm32_tm1637.h"
#include "main.h"


void _tm1637Start(void);
void _tm1637Stop(void);
void _tm1637ReadResult(void);
void _tm1637WriteByte(unsigned char b);
void _tm1637DelayUsec(unsigned int i);
void _tm1637ClkHigh(void);
void _tm1637ClkLow(void);
void _tm1637DioHigh(void);
void _tm1637DioLow(void);

// Configuration.

#define CLK_PORT GPIOC
#define DIO_PORT GPIOC
#define CLK_PIN GPIO_PIN_1
#define DIO_PIN GPIO_PIN_2
#define CLK_PORT_CLK_ENABLE __HAL_RCC_GPIOC_CLK_ENABLE
#define DIO_PORT_CLK_ENABLE __HAL_RCC_GPIOC_CLK_ENABLE

#define DISPLAY_ERR(display1,display2,display3)			(display1) = 0x50; (display2) = 0x50; (display3) = 0x79

/*	Segment map :
 * 				  A
 *				 ---
 * 			   F|	|B
 * 				|G	|
 * 				 ---
 * 			   E|	|C
 * 				|	|
 * 				 ---
 * 				  D
 *
 * 		bx 	1111 1111
 * 			SGFE DCBA
 *
 * 		S - separator
 */

const char segmentMap[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, // 0-7			[0-7]
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, // 8-9, A-F		[8-15]
    0x40,0x50,0x00									// -,r,NULL		[16-18]
};


void tm1637Init(void)
{
    CLK_PORT_CLK_ENABLE();
    DIO_PORT_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pull = GPIO_PULLUP;
    g.Mode = GPIO_MODE_OUTPUT_OD; // OD = open drain
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Pin = CLK_PIN;
    HAL_GPIO_Init(CLK_PORT, &g);
    g.Pin = DIO_PIN;
    HAL_GPIO_Init(DIO_PORT, &g);

    tm1637SetBrightness(8);
}

/*
 * Take float number and put it on display
 * If value is under -150 or over 150 it will display Err
 * For negative values it will use first 8segment as minus and 3 others as value
 * Separator will move accordinly to the value that it has to show
 *
 * @param[value] - float value to display [-150 - +150 range]
 * @return - void
 */
void tm1637DisplayFloat(float value)
{
	uint16_t v = (uint16_t) (value * 100);
	unsigned char digitArr[4];
	uint8_t SeparatorPosition = 4; // outside the range
	if (v > 0)
	{
		if (v > 15000)
		{
			// if value is under -150 then something is wrong -> display error
			DISPLAY_ERR(digitArr[0], digitArr[1], digitArr[2]);

		}
		else
		{
			// separator middle position
			SeparatorPosition = 2;
			if (v > 9999)
			{
				// move separator
				SeparatorPosition = 1;
				// cut one digit
				v /= 10;
			}
			for (int i = 0; i < 4; ++i)
			{
				digitArr[i] = segmentMap[v % 10];
				if (i == SeparatorPosition)
				{
					digitArr[i] |= 1 << 7;
				}
				v /= 10;
			}
		}
	}
	else
	{
		// for negative number we use only 3 displays (first is minus)
		// flip the sign
		v *= -1;
		SeparatorPosition = 2;
		if (v > 15000)
		{
			// if value is under -150 then something is wrong -> display error
			DISPLAY_ERR(digitArr[0],digitArr[1],digitArr[2]);

		}
			// if there is no error check further
		else
		{

			if (v > 9999)
			{
				// cut 2 digits
				v /= 100;
				// no separator
				SeparatorPosition = 4;
			}
			else if (v > 999)
			{
				// cut 1 digit
				v /= 10;
				// move separator
				SeparatorPosition = 1;
			}
			for (int i = 0; i < 3; ++i)
			{
				digitArr[i] = segmentMap[v % 10];
				if (i == SeparatorPosition)
				{
					digitArr[i] |= 1 << 7;
				}
				v /= 10;
			}
		}

		digitArr[3] = segmentMap[16]; // minus
	}


    // write prepared data
    _tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();

    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 4; ++i) {
        _tm1637WriteByte(digitArr[3 - i]);
        _tm1637ReadResult();
    }

    _tm1637Stop();
}

void tm1637DisplayDecimal(int v, int displaySeparator)
{
    unsigned char digitArr[4];
    for (int i = 0; i < 4; ++i) {
        digitArr[i] = segmentMap[v % 10];
        if (i == 2 && displaySeparator) {
            digitArr[i] |= 1 << 7;
        }
        v /= 10;
    }

    _tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();

    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 4; ++i) {
        _tm1637WriteByte(digitArr[3 - i]);
        _tm1637ReadResult();
    }

    _tm1637Stop();
}

// Valid brightness values: 0 - 8.
// 0 = display off.
void tm1637SetBrightness(char brightness)
{
    // Brightness command:
    // 1000 0XXX = display off
    // 1000 1BBB = display on, brightness 0-7
    // X = don't care
    // B = brightness
    _tm1637Start();
    _tm1637WriteByte(0x87 + brightness);
    _tm1637ReadResult();
    _tm1637Stop();
}

void _tm1637Start(void)
{
    _tm1637ClkHigh();
    _tm1637DioHigh();
    _tm1637DelayUsec(2);
    _tm1637DioLow();
}

void _tm1637Stop(void)
{
    _tm1637ClkLow();
    _tm1637DelayUsec(2);
    _tm1637DioLow();
    _tm1637DelayUsec(2);
    _tm1637ClkHigh();
    _tm1637DelayUsec(2);
    _tm1637DioHigh();
}

void _tm1637ReadResult(void)
{
    _tm1637ClkLow();
    _tm1637DelayUsec(5);
    // while (dio); // We're cheating here and not actually reading back the response.
    _tm1637ClkHigh();
    _tm1637DelayUsec(2);
    _tm1637ClkLow();
}

void _tm1637WriteByte(unsigned char b)
{
    for (int i = 0; i < 8; ++i) {
        _tm1637ClkLow();
        if (b & 0x01) {
            _tm1637DioHigh();
        }
        else {
            _tm1637DioLow();
        }
        _tm1637DelayUsec(3);
        b >>= 1;
        _tm1637ClkHigh();
        _tm1637DelayUsec(3);
    }
}

void _tm1637DelayUsec(unsigned int i)
{

//	for(uint8_t i = 0;i++ ; i<10)
//	{
//
//	}

	uint32_t delay = HAL_GetTick();

	do
	{
	}while((HAL_GetTick() - delay) < 2);
}

void _tm1637ClkHigh(void)
{
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
}

void _tm1637ClkLow(void)
{
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
}

void _tm1637DioHigh(void)
{
    HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_SET);
}

void _tm1637DioLow(void)
{
    HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_RESET);
}
