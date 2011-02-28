#ifndef __LED_H
#define __LED_H

#define led_MAX_NO_CHANNELS	2
#define led_RED				0
#define led_WHITE				1
#define led_NO_ACK				0
#define led_SEND_ACK			1
#define led_FADE_MASTER_UNDEFINED 0xFF

extern float led_PresetLevel[];
extern float led_FadeTargetLevel[];
extern float led_FadeFromLevel[];
extern float led_CurrentLevel[];
extern float led_LastLevel, led_CurFadeStep;

extern unsigned char led_CurrentPort;
extern unsigned short led_NoChannels;
extern unsigned short led_CurrentColor;
extern unsigned short led_FadeInProgress[led_MAX_NO_CHANNELS];
extern unsigned char led_DimmerTicks;
extern unsigned char led_FadeMaster;
extern unsigned char led_LevelControlGroup;

extern const char* led_ParamNames[];

enum led_CalibrationParameters {
	/* 00 */ led_None,
	/* 01 */ led_ThrottleMin,
	/* 02 */ led_ThrottleMax,
	/* 03 */ led_GearNeutral,
	/* 04 */ led_GearForward,
	/* 05 */ led_NO_CALIBRATION_PARAMS
};

extern short	engine_Calibration[];

void led_Initialize( void );

void led_SetLevel( unsigned char color, float level, unsigned char sendAck );

void led_FadeToLevel( unsigned char color, float level, float fadeSeconds );

void led_SetBacklight( event_t *event );

unsigned short led_Level( unsigned char color );

void led_Toggle( unsigned char color, float fadeTime );

void led_StopFade( unsigned char color );

float led_GetPWMLevel( unsigned char color );

void led_ProcessEvent( event_t *event, unsigned char function, unsigned char action );

void led_PowerOnTest();

void led_TaskComplete();

void led_StepDimmer( float *step, unsigned char color, unsigned char function, unsigned char event );

void led_FadeTask();

void led_PWMTask();

void led_IndicatorPWM( unsigned char run );

int led_CalibrationParams();

#endif
