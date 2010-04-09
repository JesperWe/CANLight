/*
 * hw.h
 *
 *  Created on: 2009-jun-07
 *      Author: Jesper W
 */

#include <p24hxxxx.h>
#include <pps.h>
#include <libpic30.h>
#include <generic.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "queue.h"

#ifndef HW_H_
#define HW_H_

//---------------------------------------------------------------------------------------------
// Missing in pps.h:
#define OUT_FN_PPS_OC3				0x0014				/* RPn tied to Output Compare 3 */
#define OUT_FN_PPS_OC4				0x0015				/* RPn tied to Output Compare 4 */
//---------------------------------------------------------------------------------------------

#define NOP __builtin_nop()

#define DEBUG 1

#define hw_ECAN_MODE_NORMAL	0x0
#define hw_ECAN_MODE_DISABLE	0x1

#define hw_CONFIG_MAGIC_WORD	4713

#define hw_SLOW_HEARTBEAT_MS	3000
#define hw_FAST_HEARTBEAT_MS	300

#define hw_FCY					7024531

enum hw_Functions_e {
	/* 00 */ hw_UNKNOWN,
	/* 01 */ hw_CAN_RATE,
	/* 02 */ hw_CAN_EN,
	/* 03 */ hw_LED_RED,
	/* 04 */ hw_LED_WHITE,
	/* 05 */ hw_LED1,
	/* 06 */ hw_LED2,
	/* 07 */ hw_LED3,
	/* 08 */ hw_SWITCH1,
	/* 09 */ hw_SWITCH2,
	/* 10 */ hw_SWITCH3,
	/* 11 */ hw_SWITCH4,
	/* 12 */ hw_KEY1,
	/* 13 */ hw_KEY2,
	/* 14 */ hw_KEY3,
	/* 15 */ hw_LED_LIGHT,	// Composite port RED+WHITE
	/* 16 */ hw_BACKLIGHT,	// Virtual Port.
	/* 17 */ hw_ANALOG,
	/* 18 */ hw_DIGITAL_IN,
	/* 19 */ hw_PWM1,
	/* 20 */ hw_PWM2,
	/* 21 */ hw_NoFunctions
};


enum hw_Variants {
	hw_LEDLAMP,
	hw_SWITCH,
	hw_NoVariants
};

typedef struct hw_Port_s {
   volatile unsigned int *port;
   volatile unsigned int *tris;
   int bit;
} hw_Port_t;

typedef union hw_Config_u {
	short		data[_FLASH_PAGE];
	struct {
		short	MagicWord;

		short	nmeaArbitraryAddress;
		short	nmeaIndustryGroup;
		short	nmeaVehicleSystem;
		short	nmeaFunction;
		short	nmeaFunctionInstance;
		short	nmeaManufacturerCode;
		long 	nmeaIdentityNumber;

		short 	engine_Calibration[engine_NO_CALIBRATION_PARAMS];

		short 	led_ConfigStart;
		short 	led_BacklightMultiplier;
		short 	led_BacklightOffset;
		short 	led_BacklightDaylightCutoff;
		short	led_MinimumDimmedLevel;
	};
} hw_Config_t;


//---------------------------------------------------------------------------------------------

extern const short __attribute__((space(auto_psv),aligned(_FLASH_PAGE*2))) hw_ConfigData[];
extern unsigned char 		hw_1kBuffer[1024];
extern hw_Config_t*			hw_Config;
extern _prog_addressT		hw_ConfigPtr;
extern unsigned short		hw_HeartbeatCounter;
extern unsigned short		hw_Type;
extern unsigned char 		hw_I2C_Installed;
extern unsigned char		hw_Photodetector_Installed;
extern unsigned char		hw_Throttle_Installed;
extern unsigned char		hw_Actuators_Installed;
extern unsigned char 		hw_DeviceID;
extern unsigned short		hw_PWMInverted;
extern unsigned char 		hw_DetectorADCChannel;
extern unsigned char 		hw_AutoBacklightMode;
extern const unsigned short hw_NoKeys[hw_NoVariants];
extern unsigned char 		hw_AmbientLevel;
extern unsigned char 		hw_LEDStatus;
extern unsigned char 		hw_CanSleep;
extern unsigned short		hw_SleepTimer;

unsigned int hw_ReadPort(enum hw_Functions_e port);
void hw_InputPort(enum hw_Functions_e port);
void hw_OutputPort(enum hw_Functions_e port);
void hw_WritePort(enum hw_Functions_e, int value);
void hw_Initialize( void );
void hw_ReadConfigFlash( void );
void hw_WriteConfigFlash( void );
unsigned char hw_IsPWM( unsigned short hw_Port );
void hw_Sleep( void );
void hw_AcknowledgeSwitch( unsigned char function, int setting );

void ADC_Initialize(void);
unsigned int ADC_Read( unsigned char channel );

#endif /* HW_H_ */
