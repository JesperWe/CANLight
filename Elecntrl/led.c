#include <limits.h>

#include "hw.h"
#include "config.h"
#include "events.h"
#include "schedule.h"
#include "display.h"
#include "nmea.h"
#include "menu.h"
#include "engine.h"
#include "ctrlkey.h"
#include "led.h"

#define ENABLE 		1
#define DISABLE 		0
#define CLEAR			0

// FCY / 64 = 57 578 kHz
// Divide by 576 to get PWM frequency = 100 Hz.
// FCY / 256 = 14 395 kHz
// Divide by 144 to get PWM frequency = 100 Hz.
#define led_PWM_PERIOD	1200

#define led_FADE_FREQ	25 // Match with T3CONbits.TCKPS and PR3 settings!

// Globals

unsigned short led_FadeInProgress[led_MAX_NO_CHANNELS];	// True if we are fading the channel.
unsigned short led_FadeStep[led_MAX_NO_CHANNELS];		// Current step in the fade.
unsigned short led_FadeSteps[led_MAX_NO_CHANNELS];		// Total steps in the fade.
unsigned short led_NoChannels;
unsigned short led_CurrentColor;
unsigned char led_LastControlledFunction;

float led_PresetLevel[led_MAX_NO_CHANNELS];
float led_FadeTargetLevel[led_MAX_NO_CHANNELS];
float led_FadeFromLevel[led_MAX_NO_CHANNELS];
float led_CurrentLevel[led_MAX_NO_CHANNELS];
float led_LastLevel, led_CurFadeStep;

//---------------------------------------------------------------------------------------------
// Set up PWM timers and fade LEDs to off.

void led_Initialize( void ) {
	int i;

	switch( hw_Type ) {
		case hw_LEDLAMP:	led_NoChannels = 2; break;
		case hw_SWITCH:		led_NoChannels = 1; break;
		default:			led_NoChannels = 0; return;
	}

	// OC Module defaults:
	//	Run in idle mode.
	//	Timer2 is input.

	if( led_NoChannels > 0 ) {
		OC1CONbits.OCM = 6; 		// PWM mode.
		OC1RS = led_PWM_PERIOD;
		if( led_NoChannels > 1 ) {
			OC2CONbits.OCM = 6; 		// PWM mode.
			OC2RS = led_PWM_PERIOD;
		}
	}


	// So setup Timer 2.

	T2CONbits.T32 = 0;  		// 16 bits.
	T2CONbits.TCS = 0;  		// Internal Clock.
	T2CONbits.TSIDL = 0;  		// Don't stop in idle.
	T2CONbits.TCKPS = 2;  		// Divide by 64 Prescaler.
	PR2 = led_PWM_PERIOD;
	T2CONbits.TON = 1;  		// Start Timer.

	hw_CanSleep = 1;
	hw_SleepTimer = 0;
	led_SetLevel( 0, 0.0 );
	led_SetLevel( 1, 0.0 );

	for( i=0; i<led_NoChannels; i++ ) {
		led_CurrentLevel[i] = led_FadeFromLevel[i] = led_GetPWMLevel( i );
		led_FadeInProgress[i] = 0;
	}

	led_CurrentColor = led_RED;
}


//---------------------------------------------------------------------------------------------
// Set one of the LED channels to a percentage brightness.

void led_SetLevel( unsigned char color, float level ) {

	float modLevel, lastLevel;
	short dutycycle;
	event_t response;

	lastLevel = led_CurrentLevel[color];
	led_CurrentLevel[color] = modLevel = level;

	// Attempt to linearize light output as function of duty cycle.
	// It turns out a square characteristic produces a nice feeling curve.

	modLevel = level * level;

	modLevel = modLevel * led_PWM_PERIOD;

	if( !hw_PWMInverted ) {
		dutycycle = led_PWM_PERIOD - (short)modLevel;
		if( dutycycle == led_PWM_PERIOD ) dutycycle++; // Remove last clock cycle for fully off.
	}

	else dutycycle = (short)modLevel;

	switch( color ) {
		case led_RED: { OC1RS = dutycycle; break;	}
		case led_WHITE: { OC2RS = dutycycle; break; }
	}

	if( schedule_Running == FALSE ) return;

	// Response message back to controller.
	// Going to or from level=0 (off) from/to any other level triggers a response.
	// If the composite lamp is addressed, only consider it off if both channels are off.

	if( lastLevel == 0.0 || modLevel == 0.0 ) {
		response.PGN = 0;
		response.info = 0;
		response.ctrlDev = hw_DeviceID;
		response.ctrlFunc = led_LastControlledFunction;
		response.ctrlEvent = (level == 0.0) ? e_SWITCH_OFF : e_SWITCH_ON;

		// No "off" response if other channel is still on.
		if( response.ctrlEvent == e_SWITCH_OFF
			&& led_CurrentLevel[1-color] > 0.0
			&& led_LastControlledFunction == hw_LED_LIGHT
		) return;

		nmea_SendEvent( &response );
	}

}


//---------------------------------------------------------------------------------------------
// Get level from PWM registers.

unsigned short led_PWMLevel( unsigned char color ) {
	short level;
	level = 0;
	switch( color ) {
		case led_RED:   { level = OC1RS; break; }
		case led_WHITE: { level = OC2RS; break;}
	}
	if( hw_PWMInverted ) {
		level = led_PWM_PERIOD - level;
		if( level < 0 ) level = 0;
	}
	return level;
}


//---------------------------------------------------------------------------------------------
// Get level from PWM registers as float.

float led_GetPWMLevel( unsigned char color ) {
	float level = (float)led_PWMLevel(color);
	level = ((float)led_PWM_PERIOD - level) / ((float)led_PWM_PERIOD);
	if( level > 1.0 ) level = 1.0;
	return level;
}


//---------------------------------------------------------------------------------------------
// Toggles between off and a preset level.

void led_Toggle( unsigned char color, float fadeTime ) {

	if( led_CurrentLevel[color] == 0.0 ) {
		led_FadeToLevel( color, led_PresetLevel[color], fadeTime );
	}
	else {
		led_PresetLevel[color] = led_CurrentLevel[color];
		led_FadeToLevel( color, 0.0, fadeTime );
	}
}


//---------------------------------------------------------------------------------------------
// Interrupt fade in progress.

void led_StopFade( unsigned char color ) {
	led_FadeInProgress[ color ] = DISABLE;
	led_FadeSteps[ color ] = 0;
	led_FadeStep[ color ] = 0;
}


//---------------------------------------------------------------------------------------------
// Calculate intermediate values in a smooth transition.

float led_SmoothStep( float t ) {
	return t * t * (3.0 - 2.0 * t);
}


//---------------------------------------------------------------------------------------------
// Initialize fade from current to new level.

void led_FadeToLevel( unsigned char color, float level, float fadeSeconds ) {

	led_FadeFromLevel[ color ] = led_CurrentLevel[color];
	led_FadeTargetLevel[ color ] = level;
	led_FadeInProgress[ color ] = ENABLE;
	led_FadeSteps[ color ] = fadeSeconds * (float) led_FADE_FREQ;
	led_FadeStep[ color ] = 0;
}


//---------------------------------------------------------------------------------------------
// Interrupt fade in progress.

void led_ProcessEvent( event_t *event, unsigned char function ) {

	led_LastControlledFunction = function;

	if( function != hw_LED_LIGHT ) led_CurrentColor = (function==hw_LED_RED) ? led_RED : led_WHITE;

	switch( event->ctrlEvent ) {
		case e_KEY_HOLDING: {
			ctrlkey_Holding = 1;
			if( led_CurrentLevel[led_CurrentColor] > 0.5 ) led_CurFadeStep = -0.05;
			else led_CurFadeStep = 0.05;
			break;
		}
		case e_KEY_RELEASED: {
			ctrlkey_Holding = 0;
			break;
		}
		case e_KEY_CLICKED: {
			led_LastLevel = led_CurrentLevel[led_CurrentColor];
			led_Toggle( led_CurrentColor, 3.0 );
			break;
		}
		case e_KEY_DOUBLECLICKED: {
			if( hw_Type == hw_LEDLAMP && function == hw_LED_LIGHT ) {
				led_CurrentColor = (led_CurrentColor==led_RED) ? led_WHITE : led_RED;
			}
			break;
		}
	}
}


//-------------------------------------------------------------------------------
// Some limited LED test sequences. Mainly used to verify the hardware of newly
// manufactured devices. This routine is run as a task that only executes once.

void led_PowerOnTest() {

	switch( hw_Type ) {

		case hw_LEDLAMP: {
			led_PresetLevel[ led_RED ] = 1.0;
			led_PresetLevel[ led_WHITE ] = 0.5;
			led_SetLevel( led_RED, 1.0 );
			led_SetLevel( led_WHITE, 1.0 );
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			led_FadeToLevel( led_WHITE, 0.0, 2.0 );
			break;
		}

		case hw_SWITCH: {
			hw_WritePort( hw_LED1, 1 );
			schedule_Sleep(500);
			hw_WritePort( hw_LED1, 0 );
			hw_WritePort( hw_LED2, 1 );
			schedule_Sleep(500);
			hw_WritePort( hw_LED2, 0 );
			hw_WritePort( hw_LED3, 1 );
			schedule_Sleep(500);
			hw_WritePort( hw_LED3, 0 );

			led_PresetLevel[ led_RED ] = 1.0;
			led_SetLevel( led_RED, 1.0);
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			break;
		}
	}

	schedule_Finished();
}


//-------------------------------------------------------------------------------
// This task will signal that the device has completed some significant task,
// such as reconfiguring itself.

void led_TaskComplete() {
	int i;

	for( i=0; i<4; i++ ) {
		switch( hw_Type ) {

			case hw_LEDLAMP: {
				led_PresetLevel[ led_WHITE ] = 0.5;
				schedule_Sleep(300);
				led_PresetLevel[ led_WHITE ] = 0.0;
				schedule_Sleep(300);
				break;
			}

			case hw_SWITCH: {
				hw_WritePort( hw_LED1, 1 );
				hw_WritePort( hw_LED2, 0 );
				hw_WritePort( hw_LED3, 1 );
				schedule_Sleep(300);
				hw_WritePort( hw_LED1, 0 );
				hw_WritePort( hw_LED2, 1 );
				hw_WritePort( hw_LED3, 0 );
				schedule_Sleep(300);
				hw_WritePort( hw_LED2, 0 );
				break;
			}
		}
	}
	schedule_Finished();
}


//---------------------------------------------------------------------------------------------
// Smooth lighting transitions.
//
// Should run with an Interrupt Interval of 40ms.

void led_CrossfadeTask() {
	unsigned short i, curStep;
	float fadePos, multiplier, value;

	hw_CanSleep = 1;
	hw_SleepTimer++;

	for( i=0; i<led_NoChannels; i++ ) {

		if( led_CurrentLevel[i] != 0 && led_CurrentLevel[i] != 1.0 )
			hw_CanSleep = 0;

		if( led_FadeInProgress[i] ) {

			curStep = ++led_FadeStep[i];
			fadePos = (float)curStep / (float)led_FadeSteps[i];
			multiplier = led_SmoothStep( fadePos );
			value = led_FadeFromLevel[i] + multiplier * (led_FadeTargetLevel[i] - led_FadeFromLevel[i]);
			led_SetLevel( i, value );

			if( curStep >= led_FadeSteps[i] ) {
				led_FadeInProgress[i] = DISABLE;
			}
		}
	}
}
