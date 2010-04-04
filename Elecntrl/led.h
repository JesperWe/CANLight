#ifndef __LED_H
#define __LED_H

#define led_MAX_NO_CHANNELS	2
#define led_RED				0
#define led_WHITE				1

extern float led_PresetLevel[];
extern float led_FadeTargetLevel[];
extern float led_FadeFromLevel[];
extern float led_CurrentLevel[];
extern float led_LastLevel, led_CurFadeStep;

extern unsigned char led_CurrentFunc;
extern unsigned short led_NoChannels;
extern unsigned short led_CurrentColor;
extern unsigned short led_FadeInProgress[led_MAX_NO_CHANNELS];
extern unsigned char led_DimmerTicks;
extern unsigned char led_FadeMaster;
extern unsigned char led_LevelControlGroup;

void led_Initialize( void );

void led_SetLevel( unsigned char color, float level );

void led_FadeToLevel( unsigned char color, float level, float fadeSeconds );

unsigned short led_Level( unsigned char color );

void led_Toggle( unsigned char color, float fadeTime );

void led_StopFade( unsigned char color );

float led_GetPWMLevel( unsigned char color );

void led_ProcessEvent( event_t *event, unsigned char function );

void led_PowerOnTest();

void led_TaskComplete();

void led_FadeTask();

#endif
