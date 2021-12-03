/*
 * parse.h
 *
 *  Created on: 24 cze 2021
 *      Author: ROJEK
 */

#ifndef INC_PARSE_H_
#define INC_PARSE_H_

#include "tmp102.h"

typedef enum
{
	PARSE_OK,
	PARSE_ERROR_NOCMD,
	PARSE_ERROR_2CMDS
}PARSE_STATUS;

typedef enum
{
	WAKE_UP,
	MEASURE,
	DISPLAY,
	SLEEP
}BT_COMMANDS;

#define ENDLINE '\n'

void Parse_WriteDataToBuffer(Ringbuffer_t *RecieveBuffer, uint8_t *ParseBuffer);
uint8_t Parser_Parse(uint8_t *ParseBuffer, TMP102_t *TMP102);

#endif /* INC_PARSE_H_ */
