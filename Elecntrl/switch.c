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
#include "schedule.h"
#include "nmea.h"

//---------------------------------------------------------------------------

unsigned short switch_timer = 0;	// Only one timer per device for now...
unsigned char switch_timedFunction;

//---------------------------------------------------------------------------

void switch_Acknowledge( unsigned char function, unsigned char setting ) {
	event_t response;

	// If we can't find what group we are in the event was a hw_DEVICE_ANY event.
	// Don't acknowledge in this case.
	if( functionInGroup[ function ] == 0) return;

	response.PGN = 0;
	response.info = 0;
	response.groupId = functionInGroup[ function ];
	response.ctrlDev = hw_DeviceID;
	response.ctrlFunc = function;
	response.ctrlEvent = (setting) ? e_SWITCH_ON : e_SWITCH_OFF;

	nmea_Wakeup();
	nmea_SendEvent( &response );
}

//---------------------------------------------------------------------------

void switch_TimerTask() {
	if( switch_timer > 0 ) switch_timer--;
	if( switch_timer == 0 ) {
		hw_WritePort( switch_timedFunction, 0 );
		switch_Acknowledge( switch_timedFunction, 0 );
		switch_timedFunction = 0;
		schedule_Finished();
	}
	return;
}

//---------------------------------------------------------------------------

void switch_ProcessEvent( event_t *event, unsigned char function, unsigned char action ) {
	unsigned char setting;

	// Do nothing if called on something that isn't a digital output.

	if( function < hw_LED1 ) return;
	if( function > hw_SWITCH4 ) return;

	setting = hw_ReadPort( function );

	switch( action ) {

		case a_SWITCH_ON: {
			setting = 1;
			break; }

		case a_SWITCH_OFF: {
			setting = 0;
			if( function == switch_timedFunction ) {
				switch_timer = 0;
				switch_timedFunction = 0;
			}
			break; }

		case a_TOGGLE_STATE: {
			setting = (setting == 0);
			break; }

		case a_ON_TIMER: {
			hw_OutputPort( function );
			setting = 1;

			if( switch_timer == 0 ) {
				schedule_AddTask( switch_TimerTask, schedule_SECOND );
				switch_timedFunction = function;
			}

			if( switch_timer < 40 ) switch_timer += 10;
			else switch_timer += 60;

			break;
		}
	}

	if( setting ) hw_OutputPort( function );

	hw_WritePort( function, setting );

	switch_Acknowledge( function, setting );

	return;
}
