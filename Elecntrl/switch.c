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

void switch_Acknowledge( unsigned char port, unsigned char setting ) {
	event_t response;

	response.PGN = 0;
	response.info = 0;
	response.groupId = config_CurrentTaskGroup;
	response.ctrlDev = hw_DeviceID;
	response.ctrlPort = port;
	response.ctrlEvent = (setting) ? e_SWITCHED_ON : e_SWITCHED_OFF;

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

void switch_ProcessEvent( event_t *event, unsigned char port, unsigned char action ) {
	unsigned char setting;

	// Do nothing if called on something that isn't a digital output.

	if( port < hw_LED1 ) return;
	if( port > hw_SWITCH4 ) return;

	setting = hw_ReadPort( port );

	switch( action ) {

		case a_SWITCH_ON: {
			setting = 1;
			break; }

		case a_SWITCH_OFF: {
			setting = 0;
			if( port == switch_timedFunction ) {
				switch_timer = 0;
				switch_timedFunction = 0;
			}
			break; }

		case a_TOGGLE_STATE: {
			setting = (setting == 0);
			break; }

		case a_ON_TIMER: {
			hw_OutputPort( port );
			setting = 1;

			if( switch_timer == 0 ) {
				schedule_AddTask( switch_TimerTask, schedule_SECOND );
				switch_timedFunction = port;
			}

			if( switch_timer < 40 ) switch_timer += 10;
			else switch_timer += 60;

			break;
		}
	}

	hw_OutputPort( port );

	hw_WritePort( port, setting );

	switch_Acknowledge( port, setting );

	return;
}
