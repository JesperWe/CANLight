#include <p24hxxxx.h>
#include "led.h"

// Remappable pins:
// RP20 - Red LED
// RP21 - White LEDs

#define ENABLE 			1
#define DISABLE 		0
#define CLEAR			0

// FCY / 64 = 57 578 kHz
// Divide by 576 to get PWM frequency = 100 Hz.
// FCY / 256 = 14 395 kHz
// Divide by 144 to get PWM frequency = 100 Hz.
#define led_PWM_PERIOD	1200

#define led_FADE_FREQ	25 // Match with T3CONbits.TCKPS and PR3 settings!

// Globals

unsigned short led_FadeInProgress[led_NO_CHANNELS];	// True if we are fading the channel.
unsigned short led_FadeStep[led_NO_CHANNELS];		// Current step in the fade.
unsigned short led_FadeSteps[led_NO_CHANNELS];		// Total steps in the fade.
float led_PresetLevel[led_NO_CHANNELS];
float led_FadeTargetLevel[led_NO_CHANNELS];
float led_FadeFromLevel[led_NO_CHANNELS];
float led_CurrentLevel[led_NO_CHANNELS];

unsigned char led_CanSleep;				// If all leds are fully on or off we can sleep.
unsigned short led_SleepTimer;			// Actually this sleep timer is used for all modules, but it needs to be declared somewhere...

//---------------------------------------------------------------------------------------------
// Set up PWM timers and fade LEDs to off.

void led_Initialize( void ) {

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

	TRISC = 0xCF;	// RC4 and RC5 outputs;

	// We use Timer3 for fades, set it up for level changes every 40 ms.

	T3CONbits.TCS = 0;  		// Internal Clock.
	T3CONbits.TSIDL = 1;  		// Stop in idle.
	T3CONbits.TCKPS = 3;  		// Divide by 256 Prescaler.
	PR3 = 1094;					// For 25Hz.
	T3CONbits.TON = 1;  		// Start Timer.

}


//---------------------------------------------------------------------------------------------
// Set one of the LED channels to a percentage brightness.
// Attempt to linearize light output as function of duty cycle.
// It turns out a square characteristic produces a nice feeling curve.

void led_SetLevel( unsigned char color, float level ) {

	float modLevel;
	short dutycycle;

	led_CurrentLevel[color] = level;

	modLevel = level * level;

	modLevel = modLevel * led_PWM_PERIOD;
	dutycycle = led_PWM_PERIOD - (short)modLevel;

	if( dutycycle == led_PWM_PERIOD ) dutycycle++; // Remove last clock cycle for fully off.

	switch( color ) {
		case led_RED: { OC1RS = dutycycle; break;	}
		case led_WHITE: { OC2RS = dutycycle; break; }
	}
}


//---------------------------------------------------------------------------------------------
// Get level from PWM registers.

unsigned short led_PWMLevel( unsigned char color ) {

	switch( color ) {
		case led_RED:   { return OC1RS; }
		case led_WHITE: { return OC2RS; }
	}
	return 0;
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

void led_Toggle( unsigned char color ) {

	if( led_CurrentLevel[color] == 0.0 ) {
		led_FadeToLevel( color, led_PresetLevel[color], 3.0 );
	}
	else {
		led_FadeToLevel( color, 0.0, 3.0 );
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
// Timer3 Interrupt service is responsible for smooth lighting transitions.

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt( void ) {
	unsigned short i, curStep;
	float fadePos, multiplier, value;

	_T3IE = DISABLE;

	led_CanSleep = 1;
	led_SleepTimer++;

	for( i=0; i<led_NO_CHANNELS; i++ ) {

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

	_T3IF = CLEAR;
	//if( led_FadeInProgress[0]==1 || led_FadeInProgress[1]==1 )
	_T3IE = ENABLE;
}
