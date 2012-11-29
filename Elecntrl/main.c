#include <stdio.h>

#include "hw.h"
#include "events.h"
#include "config.h"
#include "schedule.h"
#include "queue.h"
#include "led.h"
#include "nmea.h"
#include "switch.h"
#include "ctrlkey.h"
#include "display.h"
#include "menu.h"
#include "engine.h"
#include "ballast.h"


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

// PIC24HJ64GP504 Configuration Bit Settings

#include <p24Hxxxx.h>

// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)
#pragma config RBS = NO_RAM             // Boot Segment RAM Protection (No Boot RAM)

// FSS
#pragma config SWRP = WRPROTECT_OFF     // Secure Segment Program Write Protect (Secure segment may be written)
#pragma config SSS = NO_FLASH           // Secure Segment Program Flash Code Protection (No Secure Segment)
#pragma config RSS = NO_RAM             // Secure Segment Data RAM Protection (No Secure RAM)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = FRCPLL           // Oscillator Mode (Internal Fast RC (FRC) w/ PLL)
#pragma config IESO = ON                // Internal External Switch Over Mode (Start-up device with FRC, then automatically switch to user-selected oscillator source when ready)

// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Source (Primary Oscillator Disabled)
#pragma config OSCIOFNC = ON            // OSC2 Pin Function (OSC2 pin has digital I/O function)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow Only One Re-configuration)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Both Clock Switching and Fail-Safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = ON              // Watchdog Timer Enable (Watchdog timer always enabled)

// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)

// FICD
#pragma config ICS = PGD2               // Comm Channel Select (PGD1 for light, PGD2 for switch)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)


int main (void)
{
	hw_Initialize();

	if( hw_I2C_Installed ) {
		display_Initialize();
		menu_Initialize();
	}

	if( hw_Photodetector_Installed || hw_TankSender_Installed ) {
		ADC_Initialize();
	}

	if( hw_Actuators_Installed ) {
		engine_Initialize();
	}

	if( hw_Joystick_Installed ) {
		ADC_Initialize();
		engine_ThrottleInitialize();
	}

	if( hw_Type != hw_LEDLAMP ) {
		ctrlkey_Initialize();
	}

	nmea_Initialize();
	config_Initialize();
	events_Initialize();
	led_Initialize();


	schedule_Initialize();

	schedule_AddTask( config_UninitializedTask, schedule_SECOND/100 );

	schedule_AddTask( event_Task, schedule_SECOND/100 );

	schedule_AddTask( led_FadeTask, schedule_SECOND/25 );

	if( hw_I2C_Installed ) {
		schedule_AddTask( display_Task, schedule_SECOND/3 );
		schedule_AddTask( menu_Task, schedule_SECOND/30 );
	}

	if( hw_Photodetector_Installed ) {
		schedule_AddTask( display_BacklightTask, 2*schedule_SECOND );
	}

	if( hw_Actuators_Installed ) {
		schedule_AddTask( engine_ActuatorTask, schedule_SECOND/10 );
	}

	if( hw_Joystick_Installed ) {
		schedule_AddTask( engine_JoystickTask, schedule_SECOND/25 );
	}

	if( hw_NoKeys > 0 ) {
		schedule_AddTask( ctrlkey_task, schedule_SECOND/10 );
	}

	if( hw_DeviceID == 21 || hw_DeviceID == 22 ) {
		schedule_AddTask( ballast_ShutOffTask, schedule_SECOND );
	}

	schedule_Run();

	return 0;
}
