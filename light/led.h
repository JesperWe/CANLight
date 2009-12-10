#ifndef __LED_H
#define __LED_H

#include "events.h"
#include "hw.h"
#include "ctrlkey.h"

#define led_MAX_NO_CHANNELS	2
#define led_RED				0
#define led_WHITE				1

extern float led_PresetLevel[];
extern float led_FadeTargetLevel[];
extern float led_FadeFromLevel[];
extern float led_CurrentLevel[];
extern float led_LastLevel, led_CurFadeStep;

extern unsigned char led_CanSleep;
extern unsigned char led_LastControlledFunction;
extern unsigned short led_NoChannels;
extern unsigned short led_CurrentColor;
extern unsigned short led_SleepTimer;
extern unsigned short led_FadeInProgress[led_MAX_NO_CHANNELS];

void led_Initialize( void );

void led_SetLevel( unsigned char color, float level );

void led_FadeToLevel( unsigned char color, float level, float fadeSeconds );

unsigned short led_Level( unsigned char color );

void led_Toggle( unsigned char color, float fadeTime );

void led_StopFade( unsigned char color );

float led_GetPWMLevel( unsigned char color );

void led_ProcessEvent( event_t *event, unsigned char function );

void led_PowerOnTest();

void led_CrossfadeTask();

#endif
