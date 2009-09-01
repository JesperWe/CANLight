#ifndef __led_H
#define __led_H

#define led_NO_CHANNELS 2
#define led_RED			0
#define led_WHITE		1

extern float led_PresetLevel[];
extern float led_FadeTargetLevel[];
extern float led_FadeFromLevel[];
extern float led_CurrentLevel[];
extern unsigned char led_CanSleep;
extern unsigned short led_SleepTimer;

void led_Initialize( void );
void led_SetLevel( unsigned char color, float level );
void led_FadeToLevel( unsigned char color, float level, float fadeSeconds );
unsigned short led_Level( unsigned char color );
float led_GetLevel( unsigned char color );
void led_Toggle( unsigned char color );
void led_StopFade( unsigned char color );

#endif
