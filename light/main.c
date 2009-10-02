#define FCY 698800UL

#include "hw.h"
#include "led.h"
#include "nmea.h"
#include "switch.h"
#include "events.h"
#include "config.h"
#include "ctrlkey.h"

void goodnight( void );

//_FOSCSEL( FNOSC_FRCPLL )
//_FWDT( FWDTEN_OFF )
//_FICD( ICS_PGD1 )

int main (void)
{
	float newLevel;
	cfg_Event_t *listenEvent;

	hw_Initialize();

	switch( hw_Type ) {

		case hw_LEDLIGHT: {
			led_Initialize();
			led_PresetLevel[ led_RED ] = 1.0;
			led_PresetLevel[ led_WHITE ] = 0.5;
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			led_FadeToLevel( led_WHITE, 0.0, 2.0 );
			break;
		}

		case hw_SWITCH: {
			ctrlkey_Initialize();
			led_Initialize();
			led_PresetLevel[ led_RED ] = 1.0;
			led_CurrentLevel[ led_RED ] = led_GetPWMLevel( led_RED );
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			break;
		}
	}

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

		_RB14 = 0;
		//if( led_SleepTimer > 250 ) goodnight();

		eventPtr = events_Pop();
		if( eventPtr ) {

			switch( eventPtr->type ) {

				// Events that originates from this devices hardware usually
				// generate NMEA messages. We always start with a MAINTAIN_POWER
				// message to allow devices which are sleeping to have a CAN Bus wakeup.

				case e_KEY_CLICKED: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );

					if( loopbackEnabled ) {
						events_Push(
							e_NMEA_MESSAGE,
							nmea_LIGHTING_COMMAND,
							hw_DeviceID,
							eventPtr->ctrlFunc,
							e_KEY_CLICKED, 0,
							eventPtr->atTimer
						);
					}

					break;
				}

				case e_KEY_HOLDING: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );

					if( loopbackEnabled ) {
						events_Push(
							e_NMEA_MESSAGE,
							nmea_LIGHTING_COMMAND,
							hw_DeviceID,
							eventPtr->ctrlFunc,
							e_KEY_HOLDING, 0,
							eventPtr->atTimer
						);
					}

					break;
				}

				case e_KEY_RELEASED: {
					led_SleepTimer = 0;
					nmea_Wakeup();
					nmea_SendEvent( eventPtr );

					if( loopbackEnabled ) {
						events_Push(
							e_NMEA_MESSAGE,
							nmea_LIGHTING_COMMAND,
							hw_DeviceID,
							eventPtr->ctrlFunc,
							e_KEY_RELEASED, 0,
							eventPtr->atTimer
						);
					}

					break;
				}

				case e_WDT_RESET: {
					if( ctrlkey_Holding ) {
						newLevel = led_CurrentLevel[led_CurrentColor];
						newLevel += led_CurFadeStep;
						if( newLevel > 1.0 ) { led_CurFadeStep = - led_CurFadeStep; newLevel = 1.0; }
						if( newLevel < 0.0 ) { led_CurFadeStep = - led_CurFadeStep; newLevel = 0.0; }
						led_SetLevel( led_CurrentColor, newLevel );
					}
					break;
				}

				// When we get a NMEA message that we are listening to we find
				// out what function it controls, and take the appropriate action.

				case e_NMEA_MESSAGE: {

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
	nmea_ControllerMode( 1 ); // Disable

	if( led_CanSleep ) {
		asm volatile ("PWRSAV #0");
	}
	else { // Idle with PWM clocks still running.
		asm volatile ("PWRSAV #1");
	}

	_RB14 = 0;
	led_SleepTimer = 0;
	nmea_ControllerMode( 0 ); // Normal Operation
}
