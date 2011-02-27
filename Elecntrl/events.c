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
event_t *eventPtr;

unsigned char loopbackEnabled = 1;
short events_LastLevelSetInfo;
unsigned char events_LastLevelSetData;

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
	//	sprintf( line, "%04d:%02d(%01d) %02x %02d %03d", info, type, events_QueueFull, ctrlFunc, ctrlEvent, data );
	//	display_Write( line );
	//}

	// Check that the hardware that had the event is actually configured.

	if( (type == e_IO_EVENT) && (groupId == 0) ) return;

	newEvent.type = type;
	newEvent.groupId = groupId;
	newEvent.ctrlDev = ctrlDev;
	newEvent.ctrlFunc = ctrlFunc;
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
	unsigned char function;
	unsigned char takeAction;
	static event_t event;


	//if( led_SleepTimer > 250 ) hw_Sleep();

	if( queue_Receive( events_Queue, &event ) ) {

		if( event.type == e_IO_EVENT ) {
				nmea_SendKeyEvent( &event );
		}

		if( event.type == e_NMEA_MESSAGE ) {

			// Some events are not part of the configuration but have system defined actions,
			// so we first check for these.

			switch( event.ctrlEvent ) {

				case e_AMBIENT_LIGHT_LEVEL: {
					led_SetBacklight( &event );
					break;
				}

				// Intercept and store set level commands so we can show them in the
				// calibration display, regardless of configuration.

				case e_LEVEL_CHANGED: {
					events_LastLevelSetInfo = event.info;
					events_LastLevelSetData = event.data;
					break;
				}

				case e_FADE_MASTER: {
					if( hw_IsPWM(function) ) led_ProcessEvent( &event, function, a_SET_FADE_MASTER );
					break;
				}

				case e_SET_BACKLIGHT_LEVEL: {
					if( hw_Type != hw_SWITCH ) break;
					if( hw_IsPWM(function) ) led_SetBacklight( &event );
					break;
				}
			}

			// Do I have a function listening to events from the controller group?

			for( function=0; function<hw_NoFunctions; function++ ) {

				takeAction = config_GetFunctionActionFromEvent( function, event );

				if( takeAction != a_NO_ACTION ) {

					if( event.ctrlEvent == e_LEVEL_CHANGED ) {

						if( hw_IsPWM(function) ) {
							led_ProcessEvent( &event, function, a_SET_LEVEL );

							// Stay awake long enough to catch the next level change if we are
							// a slave in a dimming operation.

							hw_SleepTimer = schedule_SECOND;
							break;
						}

						// If not a PWM LED we must be an actuator.
						// In case we have a display, capture engine events and save the values for monitoring.

						if( hw_I2C_Installed ) {
							engine_Throttle = event.data;
							engine_Gear = event.ctrlFunc;
							engine_LastJoystickLevel = event.info;
						}
					}

					if( hw_IsPWM(function) ) led_ProcessEvent( &event, function, takeAction );
					if( hw_IsActuator(function) ) engine_ProcessEvent( &event, function, takeAction );
					else switch_ProcessEvent( &event, function, takeAction );
				}
			}

			// hw_SleepTimer = 0;

			// Now check for acknowledge events. Do I have a function that controls
			// this listener group?

			for( function=0; function<hw_NoFunctions; function++ ) {

				if( functionListenGroup[function] == event.groupId ) {

					switch( event.ctrlEvent ) {

						// Fade Starts: If we are originating the fade, led_FadeMaster is 0xFF until
						// the first response from a listener is received. This first responder becomes
						// the master for the rest of the fade.

						case e_FADE_START: {

							if( led_FadeMaster != 0xFF ) break;

							led_FadeMaster = event.ctrlDev;

							event.groupId = functionInGroup[function];
							event.ctrlDev = hw_DeviceID;
							event.ctrlEvent = e_FADE_MASTER;
							event.ctrlFunc = function;
							event.data = led_FadeMaster;
							event.info = 0;

							nmea_SendEvent( &event );

							break;
						}

						case e_SWITCH_ON: {
							hw_AcknowledgeSwitch( function, 1 );
							break;
						}

						case e_SWITCH_OFF: {
							hw_AcknowledgeSwitch( function, 0 );
							break;
						}

						case e_SWITCH_FAIL: { // XXX !
							break;
						}

						case e_THROTTLE_MASTER: {
							if( hw_Throttle_Installed ) engine_SetMaster( &event );
							break;
						}
					}
				}
			}
			break;
		}
	}
}
