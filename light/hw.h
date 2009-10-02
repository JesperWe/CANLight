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

#ifndef HW_H_
#define HW_H_

#define NOP __builtin_nop()

#define hw_CONFIG_MAGIC_WORD	4712

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
	hw_NoPortNames
};

enum hw_Variants {
	hw_LEDLIGHT,
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
		unsigned char cfgFile[];
	};
} hw_Config_t;


//---------------------------------------------------------------------------------------------

extern unsigned short __attribute__((space(prog),aligned(_FLASH_PAGE*2))) hw_ConfigData[];
extern hw_Config_t hw_Config;
extern _prog_addressT hw_ConfigPtr;
extern unsigned short hw_WDTCounter;
extern unsigned short hw_Type;
extern unsigned short hw_DeviceID;
extern unsigned short hw_PWMInverted;
extern const unsigned short hw_NoKeys[hw_NoVariants];

unsigned int hw_ReadPort(enum hw_PortNames port);
void hw_InputPort(enum hw_PortNames port);
void hw_OutputPort(enum hw_PortNames port);
void hw_WritePort(enum hw_PortNames, int value);
void hw_Initialize( void );
unsigned char hw_IsPWM( unsigned short hw_Port );

#endif /* HW_H_ */
