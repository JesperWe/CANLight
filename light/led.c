#include <limits.h>
#include "led.h"
#include "events.h"
#include "display.h"
#include "nmea.h"
#include "menu.h"
#include "engine.h"

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

unsigned char led_CanSleep;				// If all leds are fully on or off we can sleep.
unsigned short led_SleepTimer;			// Actually this sleep timer is used for all modules, but it needs to be declared somewhere...

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

	OC1CONbits.OCM = 6; 		// PWM mode.
	OC2CONbits.OCM = 6; 		// PWM mode.

	// So setup Timer 2.

	T2CONbits.T32 = 0;  		// 16 bits.
	T2CONbits.TCS = 0;  		// Internal Clock.
	T2CONbits.TSIDL = 0;  		// Don't stop in idle.
	T2CONbits.TCKPS = 2;  		// Divide by 64 Prescaler.
	PR2 = led_PWM_PERIOD;
	T2CONbits.TON = 1;  		// Start Timer.

	OC1RS = led_PWM_PERIOD + 1; 	// Value higher than led_PWM_PERIOD means 100% Duty Cycle, LED is OFF.
	OC2RS = led_PWM_PERIOD + 1;
	led_CanSleep = 1;
	led_SleepTimer = 0;

	// We use Timer3 for fades, set it up for level changes every 40 ms.

	T3CONbits.TCS = 0;  		// Internal Clock.
	T3CONbits.TSIDL = 1;  		// Stop in idle.
	T3CONbits.TCKPS = 3;  		// Divide by 256 Prescaler.
	PR3 = 1094;					// For 25Hz.
	T3CONbits.TON = 1;  		// Start Timer.

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

	// Response message back to controller.
	// Going to or from level=0 (off) from/to any other level triggers a response.
	// If the composite lamp is addressed, only consider it off if both channels are off.

	if( lastLevel == 0.0 || modLevel == 0.0 ) {
		response.PGN = 0;
		response.atTimer = 0;
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

	if( _T3IE == DISABLE ) {
		_T3IF = CLEAR;
		_T3IE = ENABLE;
	}
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


//---------------------------------------------------------------------------------------------
// Timer3 Interrupt service is responsible for smooth lighting transitions.
// We also use this interrupt for polling the I2C keypad if there is one attached.
//
// Timer 3 should run with an Interrupt Interval of 40ms.

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt( void ) {
	unsigned short i, curStep;
	float fadePos, multiplier, value;

	_T3IE = DISABLE;
	_T3IF = CLEAR;

	led_CanSleep = 1;
	led_SleepTimer++;

	for( i=0; i<led_NoChannels; i++ ) {

		if( led_CurrentLevel[i] != 0 && led_CurrentLevel[i] != 1.0 )
			led_CanSleep = 0;

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

	if( hw_Throttle_Installed ) {
		engine_ReadJoystickLevel();
	}

	if( hw_I2C_Installed ) {

		// We can either poll the keypad or process a pressed key in one interrupt cycle.
		// The display is too slow to poll and process in the same cycle, it will protest.

		if( display_PendingKeypress == 0 ) {

			display_PendingKeypress = display_ReadKeypad();
			if( display_PendingKeypress != 0x00 ) {

				// If MSB is set there was a negative status which means the display
				// did not ACK. It is probably busy. So we try again next interrupt cycle.

				if( (display_PendingKeypress & 0x80) != 0 ) display_PendingKeypress = 0;
			}
		}

		else {
			menu_ProcessKey( display_PendingKeypress );
			display_PendingKeypress = 0;
		}

		// Now check if we have an acive event handler function from the
		// menu state machine. This function should then be run twice per second.

		if( menu_ActiveHandler != 0 ) {
			if( (led_SleepTimer % 12) == 0 ) {
				menu_ActiveHandler();
			}
		}
	}

	_T3IE = ENABLE;
}
