/*
 * switch.c
 *
 *  Created on: 30 sep 2009
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "events.h"
#include "switch.h"
#include "led.h"

void switch_ProcessEvent( event_t *event, unsigned char function, unsigned char action ) {
	unsigned char setting;

	// Do nothing if called on something that isn't a digital output.

	if( function < hw_LED1 ) return;
	if( function > hw_SWITCH4 ) return;

	setting = hw_ReadPort( function );

	switch( action ) {
		case a_SWITCH_ON: { setting = 1; break; }
		case a_SWITCH_OFF: { setting = 0; break; }
		case a_TOGGLE_STATE: { setting = (setting == 0); break; }
	}

	hw_WritePort( function, setting );

	return;
}
