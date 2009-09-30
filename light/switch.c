/*
 * switch.c
 *
 *  Created on: 30 sep 2009
 *      Author: Jesper W
 */

#include "switch.h"

void switch_ProcessEvent( event_t *event, unsigned char function ) {
	unsigned char setting;

	// Do nothing if called on something that isn't a digital output.

	if( function < hw_LED1 ) return;
	if( function > hw_SWITCH4 ) return;

	setting = hw_ReadPort( function );

	if( event->data == e_KEY_CLICKED ) setting = (setting == 0);
	if( event->data == e_SWITCH_ON ) setting = 0;
	if( event->data == e_SWITCH_OFF ) setting = 1;

	hw_WritePort( function, setting );

	//event_Acknowledge(  );

	return;
}
