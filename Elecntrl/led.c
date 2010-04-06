#include <limits.h>
#include <math.h>

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

#define led_PWM_PERIOD	hw_FCY / 64 / 100 					// For 100Hz PWM with 1:64 prescaler.

#define led_FADE_FREQ	25

// Globals

unsigned short led_FadeInProgress[led_MAX_NO_CHANNELS];	// True if we are fading the channel.
unsigned short led_FadeStep[led_MAX_NO_CHANNELS];			// Current step in the fade.
unsigned short led_FadeSteps[led_MAX_NO_CHANNELS];			// Total steps in the fade.
unsigned short led_NoChannels;
unsigned short led_CurrentColor;
unsigned char led_DimmerTicks;
unsigned char led_CurrentFunc;
unsigned char led_FadeMaster;
unsigned char led_LevelControlGroup;

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
		case hw_LEDLAMP: {
			led_NoChannels = 2;
			OC1CONbits.OCM = 6; // PWM mode.
			OC1RS = led_PWM_PERIOD;
			OC2CONbits.OCM = 6;
			OC2RS = led_PWM_PERIOD;
			led_PresetLevel[ led_RED ] = 1.0;
			led_PresetLevel[ led_WHITE ] = 0.5;
			break;
		}
		case hw_SWITCH:	{
			led_NoChannels = 1;
			OC1CONbits.OCM = 6;		// PWM mode.
			OC1RS = led_PWM_PERIOD;
			OC2CONbits.OCM = 5;		// Continuous Pulse Mode to get interrupt at OC2RS
			OC2R = 0;
			OC2RS = led_PWM_PERIOD;
			led_PresetLevel[ led_RED ] = 1.0;
			break;
		}
		default:			led_NoChannels = 0; return;
	}

	// So setup Timer 2.

	T2CONbits.T32 = 0;  		// 16 bits.
	T2CONbits.TCS = 0;  		// Internal Clock.
	T2CONbits.TSIDL = 0;  		// Don't stop in idle.
	T2CONbits.TCKPS = 2;  		// Divide by 64 Prescaler.
	PR2 = led_PWM_PERIOD;
	T2CONbits.TON = 1;  		// Start Timer.

	hw_CanSleep = 1;
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
	unsigned short i;
	float modLevel, lastLevel;
	short dutycycle;
	event_t response;

	if( level != 0 ) hw_CanSleep = 0;

	lastLevel = led_CurrentLevel[color];
	led_CurrentLevel[color] = modLevel = level;

	// Attempt to linearize light output as function of duty cycle.
	// It turns out a near square characteristic produces a nice feeling curve.

	modLevel = pow(level, 1.8);

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

	// Check if any channel is under PWM. In that case we can't goto deep sleep.

	hw_CanSleep = 1;
	for( i=0; i<led_NoChannels; i++ ) {
		if( led_CurrentLevel[i] != 0 && led_CurrentLevel[i] != 1.0 )
			hw_CanSleep = 0;
	}

	// Don't acknowledge if we are in POST or config update.

	if( config_Invalid ) return;
	if( schedule_Running == FALSE ) return;
	if( led_CurrentFunc == 0 ) return;

	// Response message back to controller.
	// Going to or from level=0 (off) from/to any other level triggers a response.
	// If the composite lamp is addressed, only consider it off if both channels are off.

	if( lastLevel == 0.0 || modLevel == 0.0 ) {
		response.PGN = 0;
		response.info = 0;
		response.groupId = functionInGroup[ led_CurrentFunc ];
		response.ctrlDev = hw_DeviceID;
		response.ctrlFunc = led_CurrentFunc;
		response.ctrlEvent = (level == 0.0) ? e_SWITCH_OFF : e_SWITCH_ON;

		// No "off" response if other channel is still on.
		if( response.ctrlEvent == e_SWITCH_OFF
			&& led_CurrentLevel[1-color] > 0.0
			&& led_CurrentFunc == hw_LED_LIGHT
		) return;

		nmea_Wakeup();
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
	event_t fadeEvent;

	led_CurrentFunc = function;

	if( function != hw_LED_LIGHT ) led_CurrentColor = (function==hw_LED_RED) ? led_RED : led_WHITE;

	switch( event->ctrlEvent ) {
		case e_FADE_MASTER: {

			// We are not the master. Stop fade.
			if( event->data != hw_DeviceID ) led_CurFadeStep = 0;

			// We are master. Save controller group ID for set_level events.
			else led_LevelControlGroup = event->groupId;

			break;
		}
		case e_SET_LEVEL: {
			float value;
			led_DimmerTicks = 0;
			value = event->info / 1000.0;
			led_SetLevel( led_CurrentColor, value );
			break;
		}
		case e_KEY_HOLDING: {

			fadeEvent.groupId = functionInGroup[ function ];
			fadeEvent.ctrlDev = hw_DeviceID;
			fadeEvent.ctrlFunc = led_CurrentFunc;
			fadeEvent.ctrlEvent = e_FADE_START;
			fadeEvent.info = (unsigned short)(led_CurrentLevel[led_CurrentColor] * 1000);
			fadeEvent.data = led_CurrentColor;

			nmea_SendEvent( &fadeEvent );

			if( led_CurrentLevel[led_CurrentColor] > 0.5 ) led_CurFadeStep = -0.05;
			else led_CurFadeStep = 0.05;
			break;
		}
		case e_KEY_RELEASED: {
			led_CurFadeStep = 0.0;
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
		case e_KEY_TRIPLECLICKED: {
			if( led_CurrentLevel[led_CurrentColor] < 0.5 ) led_SetLevel( led_CurrentColor, 0.06 );
			else led_SetLevel( led_CurrentColor, 1.0 );
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
			led_SetLevel( led_RED, 1.0 );
			led_SetLevel( led_WHITE, 1.0 );
			led_FadeToLevel( led_RED, 0.0, 2.0 );
			led_FadeToLevel( led_WHITE, 0.0, 2.0 );
			break;
		}

		case hw_SWITCH: {
			hw_WritePort( hw_LED1, 1 );
			schedule_Sleep(schedule_SECOND/3);
			hw_WritePort( hw_LED1, 0 );
			hw_WritePort( hw_LED2, 1 );
			schedule_Sleep(schedule_SECOND/3);
			hw_WritePort( hw_LED2, 0 );
			hw_WritePort( hw_LED3, 1 );
			schedule_Sleep(schedule_SECOND/3);
			hw_WritePort( hw_LED3, 0 );

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

	for( i=0; i<3; i++ ) {
		switch( hw_Type ) {

			case hw_LEDLAMP: {
				led_SetLevel( led_WHITE, 0.5 );
				schedule_Sleep(schedule_SECOND/4);
				led_SetLevel( led_WHITE, 0.0 );
				schedule_Sleep(schedule_SECOND/4);
				break;
			}

			case hw_SWITCH: {
				hw_WritePort( hw_LED1, 1 );
				hw_WritePort( hw_LED2, 0 );
				hw_WritePort( hw_LED3, 1 );
				schedule_Sleep(schedule_SECOND/4);
				hw_WritePort( hw_LED1, 0 );
				hw_WritePort( hw_LED2, 1 );
				hw_WritePort( hw_LED3, 0 );
				schedule_Sleep(schedule_SECOND/4);
				hw_WritePort( hw_LED2, 0 );
				break;
			}
		}
	}
	schedule_Finished();
}


//-------------------------------------------------------------------------------
// Do bit-banging PWM on keypad indicator LEDs, since they are not on PWM outputs.

#define led_PWM_RESOLUTION	15

void led_PWMTask() {
	static unsigned char tickCounter;
	unsigned short level;

	if( ! hw_LEDStatus ) return;
	if( led_CurrentLevel[ led_RED ] == 0.0 ) return;
	if( led_CurrentLevel[ led_RED ] == 1.0 ) return;

	tickCounter++;
	tickCounter = tickCounter % led_PWM_RESOLUTION;

	if( tickCounter == 0 ) {
		if( hw_LEDStatus & 1 ) hw_WritePort( hw_LED1, 1 );
		if( hw_LEDStatus & 2 ) hw_WritePort( hw_LED2, 1 );
		if( hw_LEDStatus & 4 ) hw_WritePort( hw_LED3, 1 );
		return;
	}

	// We know here that we are a hw_SWITCH, and led_RED is back-light!

	level = 1 + (short)(led_CurrentLevel[ led_RED ] * (float)led_PWM_RESOLUTION);
	if( level == tickCounter ) {
		hw_WritePort( hw_LED1, 0 );
		hw_WritePort( hw_LED2, 0 );
		hw_WritePort( hw_LED3, 0 );
	}
}

//---------------------------------------------------------------------------------------------
// Smooth lighting transitions.

void led_FadeTask() {
	unsigned short i;
	unsigned short curStep;
	event_t	levelEvent;

	float fadePos, multiplier, value;

	hw_CanSleep = 1;

	for( i=0; i<led_NoChannels; i++ ) {

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

	// If we are dimming, take care of that too.
	// If we have come here we are the master device for this fade and
	// should update the network when we set levels.

	if( led_CurFadeStep != 0.0 ) {
		led_DimmerTicks++;
		led_DimmerTicks = led_DimmerTicks % 5; // 5 steps/second if fade is 25 steps.
		if( led_DimmerTicks == 0 ) {

			value = led_CurrentLevel[led_CurrentColor];
			value += led_CurFadeStep;
			if( value > 1.0 ) { led_CurFadeStep = - led_CurFadeStep; value = 1.0; }
			if( value < 0.0 ) { led_CurFadeStep = - led_CurFadeStep; value = 0.0; }

			levelEvent.groupId = led_LevelControlGroup;
			levelEvent.ctrlDev = hw_DeviceID;
			levelEvent.ctrlEvent = e_SET_LEVEL;
			levelEvent.ctrlFunc = hw_LED_LIGHT;
			levelEvent.data = led_CurrentColor;
			levelEvent.info = (short) (value * 1000);

			nmea_SendEvent( &levelEvent );

			led_SetLevel( led_CurrentColor, value );
		}
	}
}

//---------------------------------------------------------------------------------------------
// If we are running with dimmed back-light, we also need to dim the indicator LEDs.

void led_IndicatorPWM( unsigned char run ) {
	if( run ) {
		OC2R = 0;
		OC2RS = 3 * OC1RS;
		_OC2IE = 1;
		_T2IE = 1;
		_OC2IF = 0;
		_T2IF = 0;
	}
	else {
		_OC2IE = 0;
		_T2IE = 0;
	}
}

//---------------------------------------------------------------------------------------------
// Timer 2 Interrupt. Used when we are following the OC1 PWM for Status LED dimming.

void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void) {

    _T2IF = 0;

	if( ! hw_LEDStatus ) return;
	if( led_CurrentLevel[ led_RED ] == 0.0 ) return;
	if( led_CurrentLevel[ led_RED ] == 1.0 ) return;

	if( hw_LEDStatus & 1 ) hw_WritePort( hw_LED1, 1 );
	if( hw_LEDStatus & 2 ) hw_WritePort( hw_LED2, 1 );
	if( hw_LEDStatus & 4 ) hw_WritePort( hw_LED3, 1 );
}

//---------------------------------------------------------------------------------------------
// Output Compare 2 Interrupt. Used when we are following the OC1 PWM for Status LED dimming.
// Note that when OC2 is in PWM mode it does not generate interrupts.

void __attribute__((interrupt, no_auto_psv)) _OC2Interrupt(void) {

    _OC2IF = 0;

	if( ! hw_LEDStatus ) return;

	hw_WritePort( hw_LED1, 0 );
	hw_WritePort( hw_LED2, 0 );
	hw_WritePort( hw_LED3, 0 );
}
