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

//---------------------------------------------------------------------------------------------
// Globals.

queue_t* events_Queue;
event_t *eventPtr;

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
	static event_t event;

	//if( led_SleepTimer > 250 ) hw_Sleep();

	if( queue_Receive( events_Queue, &event ) ) {

		switch( event.type ) {

			case e_IO_EVENT: 	{ nmea_SendKeyEvent( &event ); break; }

			// When we get a NMEA message (that we are listening to!) we find
			// out what function it controls, and take the appropriate action.

			case e_NMEA_MESSAGE: {

				// Always listen to config file updates.

				if( event.ctrlEvent == e_CONFIG_FILE_UPDATE ) {
					config_Update( event.info );
					break;
				}

				if( (event.ctrlEvent == e_AMBIENT_LIGHT_LEVEL) && hw_AutoBacklightMode ) {
					unsigned short blLevel;
					blLevel = (2*event.info) + 10;
					if( blLevel > 0xFF ) blLevel = 0xFF;

					if( hw_I2C_Installed ) {
						display_SetBrightness( blLevel );
					}

					if( hw_Type == hw_SWITCH ) {

						// Turn off back-light during daytime.

						if( event.info > 220 ) {
							led_SetLevel( led_RED, 0.0 );
							led_IndicatorPWM( FALSE );
						}
						else {
							led_SetLevel( led_RED, (float)(blLevel)/256.0 );
							led_IndicatorPWM( TRUE );
						}
					}
				}

				// Do I have a function listening to events from the controller group?
				// XXX Also apply event filter here, it is unused at the moment!

				for( function=0; function<hw_NoFunctions; function++ ) {

					if( functionListenGroup[function] == event.groupId ) {

						switch( event.ctrlEvent ) {
							case e_KEY_CLICKED: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								else switch_ProcessEvent( &event, function );
								break;
							}
							case e_KEY_DOUBLECLICKED: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								break;
							}
							case e_KEY_TRIPLECLICKED: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								break;
							}
							case e_KEY_HOLDING: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								break;
							}
							case e_FADE_MASTER: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								break;
							}
							case e_KEY_RELEASED: {
								if( hw_IsPWM(function) ) led_ProcessEvent( &event, function );
								break;
							}
							case e_SET_LEVEL: {
								if( hw_IsPWM(function) ) {
									led_ProcessEvent( &event, function );
									break;
								}

								// If it is an engine event, save transmitted values for monitoring.
			
								if( hw_I2C_Installed ) {
									engine_Throttle = event.data;
									engine_Gear = event.ctrlFunc;
									engine_LastJoystickLevel = event.info;
								}
			
								// If we are a unit actually running an engine, do it!
			
								if( hw_Actuators_Installed ) {
									engine_RequestGear( event.ctrlFunc );
									engine_RequestThrottle( event.data );
								}
								break;
							}
						}
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
							case e_SWITCH_ON: { hw_AcknowledgeSwitch( function, 1 ); break; }
							case e_SWITCH_OFF: { hw_AcknowledgeSwitch( function, 0 ); break; }
							case e_SWITCH_FAIL: { break; }
						}
					}
				}
				break;
			}
		}
	}
}
