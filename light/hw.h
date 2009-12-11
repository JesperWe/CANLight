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

//#define DEBUG 1

#define hw_ECAN_MODE_NORMAL	0x0
#define hw_ECAN_MODE_DISABLE	0x1

#define hw_CONFIG_MAGIC_WORD	4713

#define hw_SLOW_HEARTBEAT_MS	3000
#define hw_FAST_HEARTBEAT_MS	300

enum hw_PortNames {
	hw_CAN_RATE,
	hw_CAN_EN,
	hw_LED_RED,
	hw_LED_WHITE,
	hw_LED1,
	hw_LED2,
	hw_LED3,
	hw_SWITCH1,
	hw_SWITCH2,
	hw_SWITCH3,
	hw_SWITCH4,
	hw_KEY1,
	hw_KEY2,
	hw_KEY3,
	hw_LED_LIGHT,	// Composite port RED+WHITE
	hw_BACKLIGHT,	// Virtual Port.
	hw_NoPortNames
};

enum hw_Variants {
	hw_LEDLAMP,
	hw_SWITCH,
	hw_UNKNOWN,
	hw_NoVariants
};

typedef struct hw_Port_s {
   volatile unsigned int *port;
   volatile unsigned int *tris;
   int bit;
} hw_Port_t;

typedef union hw_Config_u {
	int		data[_FLASH_ROW];
	struct {
		unsigned short MagicWord;
		unsigned short nmeaArbitraryAddress;
		unsigned short nmeaIndustryGroup;
		unsigned short nmeaVehicleSystem;
		unsigned short nmeaFunctionSwitch;
		unsigned short nmeaFunctionInstance;
		unsigned short nmeaManufacturerCode;
		unsigned long  nmeaIdentityNumber;
		unsigned short cfgSequenceNumber;
		short engine_Calibration[p_NO_CALIBRATION_PARAMS];
		unsigned char cfgFile[];
	};
} hw_Config_t;


//---------------------------------------------------------------------------------------------

extern unsigned short __attribute__((space(prog),aligned(_FLASH_PAGE*2))) hw_ConfigData[];
extern hw_Config_t 			hw_Config;
extern _prog_addressT		hw_ConfigPtr;
extern unsigned short		hw_HeartbeatCounter;
extern unsigned short		hw_Type;
extern unsigned char 		hw_I2C_Installed;
extern unsigned char		hw_Detector_Installed;
extern unsigned char		hw_Throttle_Installed;
extern unsigned char		hw_Actuators_Installed;
extern unsigned char 		hw_DeviceID;
extern unsigned short		hw_PWMInverted;
extern const unsigned short hw_NoKeys[hw_NoVariants];
extern unsigned char 		hw_AmbientLevel;

unsigned int hw_ReadPort(enum hw_PortNames port);
void hw_InputPort(enum hw_PortNames port);
void hw_OutputPort(enum hw_PortNames port);
void hw_WritePort(enum hw_PortNames, int value);
void hw_Initialize( void );
void hw_WriteConfigFlash( void );
unsigned char hw_IsPWM( unsigned short hw_Port );

void ADC_Initialize(void);
unsigned int ADC_Read( unsigned char channel );

#endif /* HW_H_ */
