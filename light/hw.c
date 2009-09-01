/*
 * hw.c
 *
 *  Created on: 2009-jun-07
 *      Author: sysadm
 */

#include <p24hxxxx.h>
#include <pps.h>
#include <libpic30.h>

#include "hw.h"
#include "nmea.h"

unsigned short __attribute__((space(prog),aligned(_FLASH_PAGE*2))) hw_ConfigData[_FLASH_ROW];
int hw_Config[_FLASH_ROW];
_prog_addressT hw_ConfigPtr;
unsigned short hw_WDTCounter = 0;

void hw_Initialize( void ) {

	CLKDIVbits.DOZE = 0b000;			// To make Fcy = Fosc/2

	AD1PCFGL = 0x1FFF;					// ANx eats ECAN1 SNAFU!

	// Set up clock oscillator.

	_PLLPRE = 2;	// Prescale = PLLPRE+2=4
	_PLLDIV = 59;	// Multiply = PLLDIV+2=61
	_PLLPOST = 3;	// Postscale = 8

	while(OSCCONbits.LOCK!=1); 	// Wait for PLL to lock

	// Check configuration area, erase it if it is non-zero but seems corrupted.
	// This can happen if the code has been recompiled and the compiler
	// has moved the hw_ConfigData area to a new address.

	_init_prog_address( hw_ConfigPtr, hw_ConfigData);
	_memcpy_p2d16( &hw_Config, hw_ConfigPtr, _FLASH_ROW );

	if( hw_Config[0] != hw_CONFIG_MAGIC_WORD ) {

		hw_Config[0] = hw_CONFIG_MAGIC_WORD;
		hw_Config[1] = nmea_ARBITRARY_ADDRESS;
		hw_Config[2] = nmea_INDUSTRY_GROUP;
		hw_Config[3] = nmea_VEHICLE_SYSTEM;
		hw_Config[4] = nmea_FUNCTION_SWITCH;
		hw_Config[5] = nmea_FUNCTION_INSTANCE;
		hw_Config[6] = nmea_MANUFACTURER_CODE;
		hw_Config[7] = (unsigned short)((nmea_IDENTITY_NUMBER & 0x001F0000) >> 16);
		hw_Config[8] = (unsigned short)(nmea_IDENTITY_NUMBER & 0xFFFF);

		_erase_flash(hw_ConfigPtr);
		_write_flash16(hw_ConfigPtr, hw_Config);
	}

	// IO setup (for SN65HVD234 CANBus driver):
	// Pin  8 - Slew Rate control pin
	// Pin  9 - Driver Enable
	// Pin 10 - CAN Tx
	// Pin 11 - CAN Rx

	TRISBbits.TRISB10 = 0;				// RS: Slew Rate control;
	TRISBbits.TRISB11 = 0;				// Driver chip enable.
	NOP; PORTBbits.RB12 = 1; NOP;		// Set pin high until ECAN module takes over.
	TRISBbits.TRISB12 = 0;				// C1TX pin as output.
	TRISBbits.TRISB13 = 1;				// C1RX pin as input.
	TRISBbits.TRISB14 = 0;				// Debug trigger output.

	PORTBbits.RB10 = 0;	NOP;			// RS = 0 -> Not in sleep mode.
	PORTBbits.RB11 = 0;	NOP;			// Chip Enable = 0, go off bus;

	// Peripheral mappings

	PPSUnLock;
	PPSOutput( PPS_OC1, PPS_RP20 ); 	// Red PWM to RP20.
	PPSOutput( PPS_OC2, PPS_RP21 ); 	// White PWM to RP21.
	RPOR6bits.RP12R = 0b10000;			// CAN Transmit to RP12.
	RPINR26bits.C1RXR = 13;				// CAN Receive from pin RP13.
	PPSLock;

	hw_WDTCounter = 0;
}
