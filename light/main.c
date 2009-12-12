#include <stdio.h>

#include "hw.h"
#include "config.h"
#include "schedule.h"
#include "queue.h"
#include "events.h"
#include "led.h"
#include "nmea.h"
#include "switch.h"
#include "ctrlkey.h"
#include "display.h"
#include "menu.h"
#include "engine.h"


/* Configuration bit settings in the IDE:
 *
 * Oscillator Mode: Fast Internal RC with PLL
 * Alternate I2C: Disabled.
 * ICS Communication Channel: PGD1 for Light, PGD2 for Switch
 * JTAG Port Disabled.
 *
 * ID Memory
 *
 * Byte 3: Unused
 *
 * Byte 2: Circuit Board Type.
 * 			= 0: Dual High Frequency Switched LED Driver.
 * 			= 1: IO Module. 4 Digital Outputs @ 25A, 3 Pushbuttons.
 * 			= 0xFF: Unknown or not programmed.
 *
 * Byte 1: Bit flags for various hardware features installed:
 *         7 - Linear Actuators
 *         6 - Joystick
 *         5 - Photo Detector
 *         4 - I2C Display
 *         3-0 - Unused
 *
 * Byte 0: NMEA-2000 Device Address (0-240)
 *
 *
 */

void config_Task();
void event_Task();
void menu_Task();
void goodnight();

int main (void)
{
	hw_Initialize();

	if( hw_I2C_Installed ) {
		display_Initialize();
		menu_Initialize();
	}

	if( hw_Detector_Installed ) {
		ADC_Initialize();
	}

	if( hw_Actuators_Installed ) {
		engine_Initialize();
	}

	if( hw_Throttle_Installed ) {
		ADC_Initialize();
		engine_ThrottleInitialize();
	}

	ctrlkey_Initialize();
	config_Initialize();
	events_Initialize();
	nmea_Initialize();
	led_Initialize();

	// Now setup tasks and start RTOS Scheduler.

	schedule_Initialize();

	schedule_AddTask( config_Task, 1 );

	schedule_AddTask( event_Task, 1 );

	schedule_AddTask( led_CrossfadeTask, 40 );

	if( hw_I2C_Installed ) {
		schedule_AddTask( display_Task, 300 );
		schedule_AddTask( menu_Task, 10 );
	}

	if( hw_Actuators_Installed ) {
		schedule_AddTask( engine_ActuatorTask, 1 );
	}

	if( hw_Throttle_Installed ) {
		schedule_AddTask( engine_ThrottleTask, 100 );
	}

	schedule_Run();

	return 0;
}


void event_Task() {
	// Send NMEA events on the bus if keys clicked,
	// and respond to incoming commands.
	// The MAINTAIN_POWER PGN should be sent before any command, to ensure
	// all sleeping units wake up before the real data arrives.

	config_Event_t *listenEvent;
	static event_t event;

	//if( led_SleepTimer > 250 ) goodnight();

	if( queue_Receive( events_Queue, &event ) ) {

		switch( event.type ) {

			// Events that originates from this device's hardware usually
			// generate NMEA messages. We always start with a MAINTAIN_POWER message
			// to allow devices which are sleeping to have a CAN Bus interrupt.

			case e_KEY_CLICKED: {
				led_SleepTimer = 0;
				nmea_Wakeup();
				nmea_SendEvent( &event );
				break;
			}

			case e_KEY_DOUBLECLICKED: {
				led_SleepTimer = 0;
				nmea_Wakeup();
				nmea_SendEvent( &event );
				break;
			}

			case e_KEY_HOLDING: {
				led_SleepTimer = 0;
				nmea_Wakeup();
				nmea_SendEvent( &event );
				break;
			}

			case e_KEY_RELEASED: {
				led_SleepTimer = 0;
				nmea_Wakeup();
				nmea_SendEvent( &event );
				break;
			}


			// When we get a NMEA message (that we are listening to!) we find
			// out what function it controls, and take the appropriate action.

			case e_NMEA_MESSAGE: {

				// Engine events are slightly special.

				if( (event.ctrlEvent == e_SET_THROTTLE) && hw_Actuators_Installed ) {
					engine_RequestGear( event.ctrlFunc );
					engine_RequestThrottle( event.data );
					break;
				}

				// Only process event this device is listening for.

				listenEvent = event_FindNextListener( config_MyEvents, &event );
				if( listenEvent == 0 ) break;

				led_SleepTimer = 0;

				if( event.PGN != nmea_LIGHTING_COMMAND ) break; // Some other NMEA device?

				do {

					if( hw_IsPWM( listenEvent->function ) )

						{ led_ProcessEvent( &event, listenEvent->function ); }

					else
						{ switch_ProcessEvent( &event, listenEvent->function ); }

					listenEvent = event_FindNextListener( listenEvent->next, &event );
				}
				while ( listenEvent != 0 );

				break;
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
