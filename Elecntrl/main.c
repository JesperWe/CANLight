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


int main (void)
{
	hw_Initialize();

	if( hw_I2C_Installed ) {
		display_Initialize();
		menu_Initialize();
	}

	if( hw_Photodetector_Installed ) {
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
	nmea_Initialize();
	config_Initialize();
	events_Initialize();
	led_Initialize();


	schedule_Initialize();

	schedule_AddTask( config_Task, 1 );

	schedule_AddTask( event_Task, 1 );

	schedule_AddTask( led_FadeTask, 40 );

	if( hw_I2C_Installed ) {
		schedule_AddTask( display_Task, 300 );
		schedule_AddTask( menu_Task, 10 );
	}

	if( hw_Photodetector_Installed ) {
		schedule_AddTask( display_BacklightTask, 2000 );
	}

	if( hw_Actuators_Installed ) {
		schedule_AddTask( engine_ActuatorTask, 1 );
	}

	if( hw_Throttle_Installed ) {
		schedule_AddTask( engine_JoystickTask, 100 );
	}

	if( hw_NoKeys > 0 ) {
		schedule_AddTask( ctrlkey_task, 50 );
	}

	schedule_Run();

	return 0;
}
