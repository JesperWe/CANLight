/*
 * events.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper W
 */

#include <stdio.h>

#include "hw.h"
#include "config.h"
#include "queue.h"
#include "events.h"
#include "display.h"
#include "nmea.h"
#include "led.h"
#include "switch.h"
#include "engine.h"
#include "schedule.h"

//---------------------------------------------------------------------------------------------
// Globals.

queue_t* events_Queue;
event_t* eventPtr;

unsigned char loopbackEnabled = 1;

//---------------------------------------------------------------------------------------------

void events_Initialize( void ) {
	events_Queue = queue_Create( events_QUEUESIZE, sizeof(event_t) );
}

//---------------------------------------------------------------------------------------------
// Add event to the queue.

void events_Push( 
		unsigned char type,
		unsigned short nmeaPGN, 
		unsigned char groupId,
		unsigned char ctrlDev, 
		unsigned char ctrlFunc, 
		unsigned char ctrlEvent,
		unsigned char data,
		unsigned short info )
{
	event_t newEvent;

	// Debug Dump...
	//if( type != e_SLOW_HEARTBEAT ){
	//	char line[30];
	//	sprintf( line, "%04d:%02d(%01d) %02x %02d %03d", info, type, events_QueueFull, ctrlPort, ctrlEvent, data );
	//	display_Write( line );
	//}

	// Check that the hardware that had the event is actually configured.

	if( (type == e_IO_EVENT) && (groupId == 0) ) return;

	newEvent.type = type;
	newEvent.groupId = groupId;
	newEvent.ctrlDev = ctrlDev;
	newEvent.ctrlPort = ctrlFunc;
	newEvent.ctrlEvent = ctrlEvent;
	newEvent.PGN = nmeaPGN;
	newEvent.data = data;
	newEvent.info = info;

    queue_Send( events_Queue, &newEvent );
}

//---------------------------------------------------------------------------------------------
// The Event Task is the main event manager. It processes both local and NMEA bus events.
//
// Send NMEA events on the bus if keys clicked,
// and respond to incoming commands.

void event_Task() {
	unsigned char port;
	unsigned char takeAction;
	static event_t event;


	if( queue_Receive( events_Queue, &event ) ) {

		if( event.type == e_IO_EVENT ) {
				nmea_SendIOEvent( &event );
		}

		if( event.type == e_NMEA_MESSAGE ) {

			// Some events are not part of the configuration but have system defined actions,
			// so we first check for these.

			switch( event.ctrlEvent ) {

				// Capture e_THROTTLE_CHANGE events if we have I2C installed.
				// This allows monitoring of system levels even if our current device
				// is not involved in the event.

				case e_THROTTLE_CHANGE: {
					if( hw_I2C_Installed ) {
						engine_Throttle = event.data;
						engine_Gear = event.ctrlPort;
						engine_LastJoystickLevel = event.info;
					}
					break;
				}

				case e_LED_LEVEL_CHANGED: {
					if( led_FadeMaster != 0 ) {
						led_ProcessEvent( &event, 0, a_SET_LEVEL );
						hw_StayAwakeTimer += schedule_SECOND;  // Wait for next level event in the fade.
					}
					return;
				}

				case e_SET_BACKLIGHT_LEVEL: {
					led_SetBacklight( &event );
					return;
				}

				case e_AMBIENT_LIGHT_LEVEL: {
					led_SetBacklight( &event );
					return;
				}

				case e_FADE_MASTER: {

					if( led_FadeMaster != led_FADE_MASTER_EXPECTED ) return;

					led_FadeMaster = event.data;

					if( led_FadeMaster != hw_DeviceID ) {

						// We are not the master. Stop fade. Also make sure we stay awake!
						led_CurFadeStep = 0;
						hw_StayAwakeTimer += schedule_SECOND;
					} else {

						// We are master. Save controller group ID for set_level events.
						led_LevelControlGroup = event.groupId;
					}
					return;
				}

				case e_THROTTLE_MASTER: {
					if( hw_Joystick_Installed ) engine_SetMaster( &event );
					return;
				}
			}

			// For all our ports we now go through the config file to see if this event should
			// cause some action to be taken.

			for( port=0; port<hw_PortCount; port++ ) {

				takeAction = config_GetPortActionFromEvent( port, &event );
				if( takeAction == a_NO_ACTION ) continue;

				// Fade Starts: If we are originating the fade, led_FadeMaster is undefined until
				// the first response from a listener is received. This first responder becomes
				// the master for the rest of the fade.

				if( takeAction == a_FADE_MASTER_ARBITRATION ) {

					// a_FADE_MASTER_ARBITRATION means this arbitration was initiated by this device,
					// so we are the ones to decide. led_FadeMaster == 0 means we are another controller
					// in this group and should shut up.

					if( led_FadeMaster != led_FADE_MASTER_EXPECTED ) break;
					led_FadeMaster = event.ctrlDev;

					event.groupId = config_CurrentGroup;
					event.ctrlDev = hw_DeviceID;
					event.ctrlEvent = e_FADE_MASTER;
					event.ctrlPort = port;
					event.data = led_FadeMaster;
					event.info = 0;

					nmea_SendEvent( &event );

					// We are not listeners to this fade ourselves, so reset led_FadeMaster.

					led_FadeMaster = 0;

					continue;
				}

				if( hw_IsLED(port) ) {
					led_ProcessEvent( &event, port, takeAction );
					continue;
				}

				if( hw_IsActuator(port) ) {
					engine_ProcessEvent( &event, port, takeAction );
					continue;
				}

				if( hw_IsSwitch(port) ) {
					switch_ProcessEvent( &event, port, takeAction );
					continue;
				}

				switch( takeAction ) {
					case a_SWITCH_ON: {
						hw_AcknowledgeSwitch( port, 1 );
						break;
					}

					case a_SWITCH_OFF: {
						hw_AcknowledgeSwitch( port, 0 );
						break;
					}
				}
			}
		}
	}
}
