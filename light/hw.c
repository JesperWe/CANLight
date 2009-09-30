/*
 * hw.c
 *
 *  Created on: 2009-jun-07
 *      Author: sysadm
 */

#include "hw.h"
#include "nmea.h"

unsigned short __attribute__((space(prog),aligned(_FLASH_PAGE*2)))
						hw_ConfigData[_FLASH_ROW];

hw_Config_t hw_Config;

_prog_addressT			hw_ConfigPtr;
unsigned short 			hw_WDTCounter = 0;
unsigned short			hw_PWMInverted = 0;

unsigned short			hw_Type;


//-------------------------------------------------------------------------------
// Structures for hardware dependent I/O pin maniulation

static const hw_Port_t hw_Port[hw_NoVariants][hw_NoPortNames] =
{{
	{ &PORTB, &TRISB, 10 }, // CAN_EN
	{ &PORTB, &TRISB, 11 },	// CAN_RATE
	{ &PORTC, &TRISC, 4 },	// LED_RED
	{ &PORTC, &TRISC, 5 },	// LED_WHITE
	{ &PORTB, &TRISB, 0 },	// LED1
	{ &PORTB, &TRISB, 0 },	// LED2
	{ &PORTB, &TRISB, 0 },	// LED3
	{ &PORTB, &TRISB, 0 },	// SWITCH1
	{ &PORTB, &TRISB, 0 },	// SWITCH2
	{ &PORTB, &TRISB, 0 },	// SWITCH3
	{ &PORTB, &TRISB, 0 },	// SWITCH4
	{ &PORTB, &TRISB, 0 },	// KEY1
	{ &PORTB, &TRISB, 0 },	// KEY2
	{ &PORTB, &TRISB, 0 }	// KEY3
},

{
	{ &PORTA, &TRISA, 1 },	// CAN_EN
	{ &PORTB, &TRISB, 1 },	// CAN_RATE
	{ &PORTB, &TRISB, 5 },	// LED_RED
	{ &PORTB, &TRISB, 0 },	// LED_WHITE
	{ &PORTA, &TRISA, 9 },	// LED1
	{ &PORTC, &TRISC, 4 },	// LED2
	{ &PORTC, &TRISC, 6 },	// LED3
	{ &PORTB, &TRISB, 0 },	// SWITCH1
	{ &PORTB, &TRISB, 0 },	// SWITCH2
	{ &PORTB, &TRISB, 0 },	// SWITCH3
	{ &PORTB, &TRISB, 0 },	// SWITCH4
	{ &PORTC, &TRISC, 3 },	// KEY1
	{ &PORTC, &TRISC, 5 },	// KEY2
	{ &PORTC, &TRISC, 7 }	// KEY3
},

{
	{ &PORTA, &TRISA, 0 },
	{ &PORTA, &TRISA, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 },
	{ &PORTB, &TRISB, 0 }
}};

const unsigned short hw_NoKeys[hw_NoVariants] = { 0, 3, 0 };


unsigned int hw_ReadPort(enum hw_PortNames port) {
    return (*(hw_Port[hw_Type][port].port) >> hw_Port[hw_Type][port].bit) & 1;
}

void hw_OutputPort(enum hw_PortNames port) {
	*(hw_Port[hw_Type][port].tris) &= ~(1 << hw_Port[hw_Type][port].bit);
}

void hw_InputPort(enum hw_PortNames port) {
	*(hw_Port[hw_Type][port].port) |= (1 << hw_Port[hw_Type][port].bit);
}

void hw_WritePort(enum hw_PortNames port, int value) {
	NOP;
	if (value) *(hw_Port[hw_Type][port].port) |= (1 << hw_Port[hw_Type][port].bit);
	else *(hw_Port[hw_Type][port].port) &= ~(1 << hw_Port[hw_Type][port].bit);
	NOP;
}


//-------------------------------------------------------------------------------

void hw_Initialize( void ) {
	DWORD_VAL fidc, fidc_data;
	short fidc_ics;

	CLKDIVbits.DOZE = 0;			// To make Fcy = Fosc/2

	AD1PCFGL = 0x1FFF;					// ANx eats ECAN1 SNAFU!

	// Set up clock oscillator.

	_PLLPRE = 2;	// Prescale = PLLPRE+2=4
	_PLLDIV = 59;	// Multiply = PLLDIV+2=61
	_PLLPOST = 3;	// Postscale = 8

	while(OSCCONbits.LOCK!=1); 	// Wait for PLL to lock

	// Find out what hardware we are using.
	// Currently we have two versions: Lamp and Switch/Controller.
	// These units use different ICD Serial ports, so we can find out
	// what we are from the _FICD<ICS> field.

	hw_Type = hw_UNKNOWN;
	fidc.Val = 0xF8000E;

	TBLPAG = fidc.word.HW;

	fidc_data.word.HW = __builtin_tblrdh(fidc.word.LW);
	fidc_data.word.LW = __builtin_tblrdl(fidc.word.LW);

	fidc_ics = fidc_data.byte.LB & 0x03; // Filter out ICS bits.

	if( fidc_ics == 0x3 ) hw_Type = hw_LEDLIGHT;
	if( fidc_ics == 0x2 ) hw_Type = hw_SWITCH;

	// Check configuration area, erase it if it is non-zero but seems corrupted.
	// This can happen if the code has been recompiled and the compiler
	// has moved the hw_ConfigData area to a new address.

	_init_prog_address( hw_ConfigPtr, hw_ConfigData);
	_memcpy_p2d16( &hw_Config, hw_ConfigPtr, _FLASH_ROW );

	if( hw_Config.data[0] != hw_CONFIG_MAGIC_WORD ) {

		hw_Config.data[0] = hw_CONFIG_MAGIC_WORD;
		hw_Config.data[1] = nmea_ARBITRARY_ADDRESS;
		hw_Config.data[2] = nmea_INDUSTRY_GROUP;
		hw_Config.data[3] = nmea_VEHICLE_SYSTEM;
		hw_Config.data[4] = nmea_FUNCTION_SWITCH;
		hw_Config.data[5] = nmea_FUNCTION_INSTANCE;
		hw_Config.data[6] = nmea_MANUFACTURER_CODE;
		hw_Config.data[7] = (unsigned short)((nmea_IDENTITY_NUMBER & 0x001F0000) >> 16);
		hw_Config.data[8] = (unsigned short)(nmea_IDENTITY_NUMBER & 0xFFFF);

		_erase_flash(hw_ConfigPtr);
		_write_flash16(hw_ConfigPtr, hw_Config.data);
	}

	// IO setup for SN65HVD234 CANBus driver.
	// The wiring is different on different versions of the hardware.

	// hw_Type					hw_LEDLIGHT		hw_SWITCH
	//--------------------------------------------------------------
	// Slew Rate control pin	Pin  8 RB10		Pin 20 RA1
	// Driver Enable			Pin  9 RB11		Pin 22 RB1
	// CAN Tx					Pin 10 RB12		Pin 10 RB12
	// CAN Rx					Pin 11 RB13		Pin 21 RB0

	TRISBbits.TRISB14 = 0;				// Debug trigger output.

	hw_OutputPort( hw_CAN_EN );
	hw_OutputPort( hw_CAN_RATE );

	hw_WritePort( hw_CAN_EN, 0 );		// Chip Enable = 0, go off bus.
	hw_WritePort( hw_CAN_RATE, 0 );		// RS = 0 -> Not in sleep mode.

	// Peripheral mappings

	switch( hw_Type ) {

		case hw_LEDLIGHT: {

			hw_OutputPort( hw_LED_RED );
			hw_OutputPort( hw_LED_WHITE );

			PPSUnLock;
			PPSOutput( PPS_OC1, PPS_RP20 ); 	// Red PWM to RP20.
			PPSOutput( PPS_OC2, PPS_RP21 ); 	// White PWM to RP21.
			RPOR6bits.RP12R = 0x10;			// CAN Transmit to RP12.
			RPINR26bits.C1RXR = 13;				// CAN Receive from pin RP13.
			PPSLock;
			break;
		}

		case hw_SWITCH: {

			hw_OutputPort( hw_LED_RED );
			hw_OutputPort( hw_LED1 );
			hw_OutputPort( hw_LED2 );
			hw_OutputPort( hw_LED3 );

			PPSUnLock;
			PPSOutput( PPS_OC1, PPS_RP5 );	 	// Red Backlight PWM to RP5.
			hw_PWMInverted = 1;
			RPOR6bits.RP12R = 0x10;			// CAN Transmit to RP12.
			RPINR26bits.C1RXR = 0;				// CAN Receive from pin RP0.
			PPSLock;
			break;
		}
	}

	hw_WDTCounter = 0;

}


//-------------------------------------------------------------------------------

unsigned char hw_IsPWM( unsigned short hw_Port ) {
	if( hw_Port == hw_LED_RED ) return 1;
	if( hw_Port == hw_LED_WHITE ) return 1;
	return 0;
}


//-------------------------------------------------------------------------------

