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
unsigned char led_CurrentPort;
unsigned char led_FadeMaster;
unsigned char led_LevelControlGroup;

float led_PresetLevel[led_MAX_NO_CHANNELS];
float led_FadeTargetLevel[led_MAX_NO_CHANNELS];
float led_FadeFromLevel[led_MAX_NO_CHANNELS];
float led_CurrentLevel[led_MAX_NO_CHANNELS];
float led_LastLevel;
float led_CurFadeStep;

const char* led_ParamNames[] = {
	"Light Calibration",
	"Backlight Multiply",
	"Backlight offset",
	"Backlight Day Cutoff",
	"Lamp Minimum Level",
};

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
			led_PresetLevel[ led_RED ] = 0.1;
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
	led_SetLevel( 0, 0.0, led_NO_ACK );
	led_SetLevel( 1, 0.0, led_NO_ACK );

	for( i=0; i<led_NoChannels; i++ ) {
		led_CurrentLevel[i] = led_FadeFromLevel[i] = led_GetPWMLevel( i );
		led_FadeInProgress[i] = 0;
	}

	led_CurrentColor = led_RED;
}


//---------------------------------------------------------------------------------------------
// Set one of the LED channels to a percentage brightness.

void led_SetLevel( unsigned char color, float level, unsigned char sendAck ) {
	unsigned short i;
	float modLevel, lastLevel;
	short dutycycle;
	event_t response;

	if( level != 0 ) hw_CanSleep = 0;

	lastLevel = led_CurrentLevel[color];
	led_CurrentLevel[color] = modLevel = level;

	// Attempt to linearize light output as function of duty cycle.
	// It turns out a near square characteristic produces a nice feeling curve.

	modLevel = (0.1 + 0.9*level) * level;

	modLevel = modLevel * led_PWM_PERIOD;

	if( ! hw_PWMInverted ) {
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

	if( ! sendAck ) return;
	if( hw_Type == hw_SWITCH && color == led_RED ) return; // Don't Ack for backlight!

	// Response message back to controller.
	// Going to or from level=0 (off) from/to any other level triggers a response.
	// If the composite lamp is addressed, only consider it off if both channels are off.

	if( lastLevel == 0.0 || modLevel == 0.0 ) {

		// If we can't find what group we are in the event was a hw_DEVICE_ANY event.
		// Don't acknowledge in this case.
		if( config_CurrentGroup == 0) return;

		response.PGN = 0;
		response.info = 0;
		response.groupId = config_CurrentGroup;
		response.ctrlDev = hw_DeviceID;
		response.ctrlPort = led_CurrentPort;
		response.ctrlEvent = (level == 0.0) ? e_SWITCH_OFF : e_SWITCH_ON;

		// No "off" response if other channel is still on.
		if( response.ctrlEvent == e_SWITCH_OFF
			&& led_CurrentLevel[1-color] > 0.0
			&& led_CurrentPort == hw_LED_LIGHT
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

void led_SetBacklight( event_t *event ) {
	float value;
	unsigned short ambientLevel;

	if( hw_Type != hw_SWITCH ) return; // Doesn't have backlight.

	switch( event->ctrlEvent ) {

		case e_SET_BACKLIGHT_LEVEL: {
			value = event->info / 1000.0;
			ambientLevel = event->info / 4;
			hw_AutoBacklightMode = FALSE;
			break;
		}

		case e_AMBIENT_LIGHT_LEVEL: {
			if( ! hw_AutoBacklightMode ) return;
			if( event->info > hw_Config->led_BacklightDaylightCutoff ) {
				value = 0.0;
			}
			else {
				ambientLevel = hw_Config->led_BacklightMultiplier * event->info;
				ambientLevel += hw_Config->led_BacklightOffset;
				if( ambientLevel > 0xFF ) ambientLevel = 0xFF;
				value = (float)(ambientLevel)/256.0;
			}
			break;
		}

		case e_KEY_CLICKED: {
			hw_AutoBacklightMode = FALSE;
			if( led_CurrentLevel[led_RED] == 0.0 ) {
				value = led_LastLevel;
			}
			else {
				led_LastLevel = led_CurrentLevel[led_RED];
				value = 0;
			}
			break;
		}

		case e_KEY_DOUBLECLICKED: {
			hw_AutoBacklightMode = TRUE;
			break;
		}

		case e_KEY_TRIPLECLICKED: {
			hw_AutoBacklightMode = FALSE;
			value = 0.01;
			break;
		}

		default: return;
	}

	if( hw_I2C_Installed ) {
		display_SetBrightness( ambientLevel );
	}

	if( value == 0 ) {
		led_SetLevel( led_RED, 0.0, led_NO_ACK );
		led_IndicatorPWM( FALSE );
	}
	else {
		led_SetLevel( led_RED, value, led_NO_ACK );
		led_IndicatorPWM( TRUE );
	}
}


//---------------------------------------------------------------------------------------------


void led_ProcessEvent( event_t *event, unsigned char port, unsigned char action ) {
	event_t fadeEvent;

	led_CurrentPort = port;

	if( port != hw_LED_LIGHT ) led_CurrentColor = (port==hw_LED_RED) ? led_RED : led_WHITE;

	switch( action ) {
		case a_SET_LEVEL: {
			float value;
			led_DimmerTicks = 0;
			value = event->info / 1000.0;
			led_CurrentColor = event->data;
			led_SetLevel( led_CurrentColor, value, led_NO_ACK );
			break;
		}
		case a_START_FADE: {

			fadeEvent.groupId = config_CurrentGroup;
			fadeEvent.ctrlDev = hw_DeviceID;
			fadeEvent.ctrlPort = led_CurrentPort;
			fadeEvent.ctrlEvent = e_FADE_START;
			fadeEvent.info = (unsigned short)(led_CurrentLevel[led_CurrentColor] * 1000);
			fadeEvent.data = led_CurrentColor;

			nmea_SendEvent( &fadeEvent );

			if( led_CurrentLevel[led_CurrentColor] > 0.5 ) led_CurFadeStep = -0.05;
			else led_CurFadeStep = 0.05;
			break;
		}
		case a_STOP_FADE: {
			led_CurFadeStep = 0.0;
			break;
		}
		case a_TOGGLE_STATE: {
			led_LastLevel = led_CurrentLevel[led_CurrentColor];
			led_Toggle( led_CurrentColor, 1.5 );
			break;
		}
		case a_CHANGE_COLOR: {
			if( hw_Type == hw_LEDLAMP && port == hw_LED_LIGHT ) {
				led_CurrentColor = (led_CurrentColor==led_RED) ? led_WHITE : led_RED;
			}
			break;
		}
		case a_GOTO_MINIMUM: {
			if( led_CurrentLevel[led_CurrentColor] < 0.5 ) {
				led_SetLevel( led_CurrentColor, (float)hw_Config->led_MinimumDimmedLevel / 100.0, led_NO_ACK );
			}

			else {
				led_SetLevel( led_CurrentColor, 1.0, led_NO_ACK );
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
			led_SetLevel( led_RED, 1.0, led_NO_ACK );
			led_SetLevel( led_WHITE, 1.0, led_NO_ACK );
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

			led_SetLevel( led_RED, 1.0, led_NO_ACK);
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

	for( i=0; i<schedule_Parameter; i++ ) {
		switch( hw_Type ) {

			case hw_LEDLAMP: {
				led_SetLevel( led_WHITE, 0.5, led_NO_ACK );
				schedule_Sleep(schedule_SECOND/4);
				led_SetLevel( led_WHITE, 0.0, led_NO_ACK );
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
	schedule_Parameter = 0;
	schedule_Finished();
}

//---------------------------------------------------------------------------------------------
// Do one dimmer step, change direction if 0% or 100% reached.
// Update calling parameter with new direction in that case.

void led_StepDimmer( float *step, unsigned char color, unsigned char function, unsigned char event ) {
	float value;
	event_t	levelEvent;

	value = led_CurrentLevel[color];
	value += *step;
	if( value > 1.0 ) { *step = - *step; value = 1.0; }
	if( value < 0.0 ) { *step = - *step; value = 0.0; }

	levelEvent.groupId = led_LevelControlGroup;
	levelEvent.ctrlDev = hw_DeviceID;
	levelEvent.ctrlEvent = event;
	levelEvent.ctrlPort = function;
	levelEvent.data = color;
	levelEvent.info = (short) (value * 1000);

	nmea_SendEvent( &levelEvent );
}

//---------------------------------------------------------------------------------------------
// Smooth lighting transitions.

void led_FadeTask() {
	unsigned short i;
	unsigned short curStep;

	float fadePos, multiplier, value;

	hw_CanSleep = 1;

	for( i=0; i<led_NoChannels; i++ ) {

		if( led_FadeInProgress[i] ) {

			curStep = ++led_FadeStep[i];
			fadePos = (float)curStep / (float)led_FadeSteps[i];
			multiplier = led_SmoothStep( fadePos );
			value = led_FadeFromLevel[i] + multiplier * (led_FadeTargetLevel[i] - led_FadeFromLevel[i]);
			led_SetLevel( i, value, led_SEND_ACK );

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

		if( led_DimmerTicks == 0 ) led_StepDimmer( &led_CurFadeStep, led_CurrentColor, hw_LED_LIGHT, e_LED_LEVEL_CHANGED );
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

		// Now make sure current settings are reflected at outputs.
		// (PWM stops at all indicators off most of the time).

		if( hw_LEDStatus & 1 ) hw_WritePort( hw_LED1, 1 );
		if( hw_LEDStatus & 2 ) hw_WritePort( hw_LED2, 1 );
		if( hw_LEDStatus & 4 ) hw_WritePort( hw_LED3, 1 );
	}
}

//---------------------------------------------------------------------------------------------
// Timer 2 Interrupt. Used when we are following the OC1 PWM for Status LED dimming.

void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void) {

    _T2IF = 0;
    _T2IE = 0;

	if( ! hw_LEDStatus ) goto done;
	if( led_CurrentLevel[ led_RED ] == 0.0 ) goto done;
	if( led_CurrentLevel[ led_RED ] == 1.0 ) goto done;

	if( hw_LEDStatus & 1 ) hw_WritePort( hw_LED1, 1 );
	if( hw_LEDStatus & 2 ) hw_WritePort( hw_LED2, 1 );
	if( hw_LEDStatus & 4 ) hw_WritePort( hw_LED3, 1 );

done:
	_T2IE= 1;
}


//--------------------------------------------------------------------------------------------

int led_CalibrationParams() {

	if( menu_ActiveHandler == 0 ) {
		menu_ActiveHandler = led_CalibrationParams;
		hw_ReadConfigFlash();
	}

	return menu_ParameterSetter( led_ParamNames, led_NO_CALIBRATION_PARAMS, &(hw_Config->led_ConfigStart) );
}

//---------------------------------------------------------------------------------------------
// Output Compare 2 Interrupt. Used when we are following the OC1 PWM for Status LED dimming.
// Note that when OC2 is in PWM mode it does not generate interrupts.

void __attribute__((interrupt, no_auto_psv)) _OC2Interrupt(void) {

    _OC2IF = 0;

	if( ! hw_LEDStatus ) return;

    _OC2IE = 0;
	hw_WritePort( hw_LED1, 0 );
	hw_WritePort( hw_LED2, 0 );
	hw_WritePort( hw_LED3, 0 );
    _OC2IE = 1;
}
