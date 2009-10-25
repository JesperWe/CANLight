#include <stdio.h>

#include "hw.h"
#include "led.h"
#include "nmea.h"
#include "switch.h"
#include "events.h"
#include "config.h"
#include "ctrlkey.h"
#include "display.h"
#include "menu.h"
#include "engine.h"

void goodnight( void );

int main (void)
{
	float newLevel;
	cfg_Event_t *listenEvent;

	hw_Initialize();

	if( hw_I2C_Installed ) {
		display_Initialize();
		menu_Initialize();
	}

	if( hw_Detector_Installed || hw_Throttle_Installed ) {
		ADC_Initialize();
	}

	if( hw_Actuators_Installed ) {
		engine_Initialize();
	}

	switch( hw_Type ) {

		case hw_LEDLAMP: {
			led_Initialize();
			led_PresetLevel[ led_RED ] = 1.0;
			led_PresetLevel[ led_WHITE ] = 0.5;
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			led_FadeToLevel( led_WHITE, 0.0, 2.0 );
			break;
		}

		case hw_SWITCH: {
			led_Initialize();
			led_PresetLevel[ led_RED ] = 1.0;
			led_CurrentLevel[ led_RED ] = led_GetPWMLevel( led_RED );
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			break;
		}
	}

	ctrlkey_Initialize();
	cfg_Initialize();
	events_Initialize();
	nmea_Initialize();

	// No point in running if we have no valid system configuration,
	// so stay here waiting for one to arrive and flash something to show
	// we are waiting.

	while( ! cfg_Valid ) {
		unsigned long interval;
		for( interval=0; interval < 200000; interval++ ) __delay32(11);
		led_SetLevel( led_RED, 0.3 );
		for( interval=0; interval < 30000; interval++ ) __delay32(11);
		led_SetLevel( led_RED, 0.0 );
	}

	// Main event loop. Send NMEA events on the bus if keys clicked,
	// and respond to incoming commands.
	// The MAINTAIN_POWER PGN should be sent before any command, to ensure
	// all sleeping units wake up before the real data arrives.

	while(1) {

		//_RB14 = 0;
		//if( led_SleepTimer > 250 ) goodnight();

		eventPtr = events_Pop();
		if( eventPtr ) {

			switch( eventPtr->type ) {

				// Events that originates from this device's hardware usually
				// generate NMEA messages. We always start with a MAINTAIN_POWER message
				// to allow devices which are sleeping to have a CAN Bus interrupt.

				case e_KEY_CLICKED: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );
					break;
				}

				case e_KEY_DOUBLECLICKED: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );
					break;
				}

				case e_KEY_HOLDING: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );
					break;
				}

				case e_KEY_RELEASED: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );
					break;
				}

				// Step intensity levels for channels that are being faded up or down.
				// XXX Watchdog reset too.

				case e_FAST_HEARTBEAT: {
					if( ctrlkey_Holding ) {
						newLevel = led_CurrentLevel[led_CurrentColor];
						newLevel += led_CurFadeStep;
						if( newLevel > 1.0 ) { led_CurFadeStep = - led_CurFadeStep; newLevel = 1.0; }
						if( newLevel < 0.0 ) { led_CurFadeStep = - led_CurFadeStep; newLevel = 0.0; }
						led_SetLevel( led_CurrentColor, newLevel );
					}
					break;
				}

				case e_SLOW_HEARTBEAT: {
					{
						if( hw_Detector_Installed ) {
							unsigned short pvVoltage;
							//char line1[20];
							event_t	ambientEvent;

							pvVoltage = ADC_Read(11);

							// Set system backlight level based on ambient light.

							pvVoltage = pvVoltage >> 2;
							if( hw_AmbientLevel != pvVoltage ) {

							}
								hw_AmbientLevel = pvVoltage;

								ambientEvent.PGN = 0;
								ambientEvent.atTimer = 0;
								ambientEvent.ctrlDev = hw_DeviceID;
								ambientEvent.ctrlFunc = hw_BACKLIGHT;
								ambientEvent.ctrlEvent = e_AMBIENT_LIGHT_LEVEL;
								ambientEvent.data = hw_AmbientLevel;

								nmea_SendEvent( &ambientEvent );

								if( hw_I2C_Installed ) {
									unsigned short blLevel;
									blLevel = (2*hw_AmbientLevel) + 10;
									if( blLevel > 0xFF ) blLevel = 0xFF;
									//display_Home();
									//sprintf( line1, "v = %04d, bl = %03d", hw_AmbientLevel, blLevel );
									//display_Write( line1 );
									display_SetBrightness( blLevel );
							}
						}
					}
					break;
				}


				// When we get a NMEA message (that we are listening to!) we find
				// out what function it controls, and take the appropriate action.

				case e_NMEA_MESSAGE: {

					// Engine events are slightly special.

					if( (eventPtr->ctrlEvent == e_SET_THROTTLE) && hw_Actuators_Installed ) {
						engine_RequestGear( eventPtr->ctrlFunc );
						engine_RequestThrottle( eventPtr->data );
						break;
					}

					// Only process event this device is listening for.

					listenEvent = event_FindNextListener( cfg_MyEvents, eventPtr );
					if( listenEvent == 0 ) break;

					led_SleepTimer = 0;

					if( eventPtr->PGN != nmea_LIGHTING_COMMAND ) break; // Some other NMEA device?

					do {

						if( hw_IsPWM( listenEvent->function ) )

							{ led_ProcessEvent( eventPtr, listenEvent->function ); }

						else
							{ switch_ProcessEvent( eventPtr, listenEvent->function ); }

						listenEvent = event_FindNextListener( listenEvent->next, eventPtr );
					}
					while ( listenEvent != 0 );

					break;
				}
			}
		}
	}
}


void goodnight( void ) {

	_RB14 = 1;

	nmea_ControllerMode( hw_ECAN_MODE_DISABLE );

	if( led_CanSleep ) {
		asm volatile ("PWRSAV #0");
	}
	else { // Idle with PWM clocks still running.
		asm volatile ("PWRSAV #1");
	}

	_RB14 = 0;
	led_SleepTimer = 0;

	nmea_ControllerMode( hw_ECAN_MODE_NORMAL );
}
