/*
 * hw.c
 *
 *  Created on: 2009-jun-07
 *      Author: sysadm
 */

#include "hw.h"
#include "config.h"
#include "nmea.h"
#include "schedule.h"
#include "queue.h"
#include "display.h"

const short __attribute__((space(auto_psv),aligned(_FLASH_PAGE*2)))
hw_ConfigData[_FLASH_PAGE * 2];

// Define 1k of general purpose RAM. Note that theoretically some modules risk trying to
// use this area simultaneously. For example sending a config file update on the NMEA bus
// at the same time as the I2C display is editing config parameters will completely
// mess up this buffer!

unsigned char hw_1kBuffer[1024];

hw_Config_t* hw_Config;

_prog_addressT hw_ConfigPtr;
unsigned short hw_HeartbeatCounter = 0;
unsigned short hw_PWMInverted = 0;

unsigned short hw_Type;
unsigned char hw_I2C_Installed = 0;
unsigned char hw_Photodetector_Installed = 0;
unsigned char hw_Joystick_Installed = 0;
unsigned char hw_TankSender_Installed = 0;
unsigned char hw_Ballast_Cntrl = 0;
unsigned char hw_Actuators_Installed = 0;
unsigned char hw_ConfigByte = 0;
unsigned char hw_DetectorADCChannel;
unsigned char hw_AutoBacklightMode = TRUE;
unsigned char hw_AmbientLevel;
unsigned char hw_LEDStatus;

unsigned char hw_DeviceID;

unsigned char hw_AmbientLevel;

unsigned char hw_CPUStopPossible; // If all PWM outputs are fully on or off we can sleep.
unsigned short hw_StayAwakeTimer; // Count ticks before we can go to sleep.


//-------------------------------------------------------------------------------
// Structures for hardware dependent I/O pin manipulation

static const hw_Port_t hw_Port[hw_NoVariants][hw_PortCount] =
{{
	{ &PORTA, &TRISA, 0 },  // Unused
	{ &PORTB, &TRISB, 10 }, // CAN_RATE
	{ &PORTB, &TRISB, 11 },	// CAN_EN
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
	{ &PORTA, &TRISA, 0 },  // Unused
	{ &PORTA, &TRISA, 1 },	// CAN_RATE
	{ &PORTB, &TRISB, 1 },	// CAN_EN
	{ &PORTB, &TRISB, 5 },	// LED_RED
	{ &PORTB, &TRISB, 0 },	// LED_WHITE
	{ &PORTA, &TRISA, 9 },	// LED1
	{ &PORTC, &TRISC, 4 },	// LED2
	{ &PORTC, &TRISC, 6 },	// LED3
	{ &PORTA, &TRISA, 8 },	// SWITCH1
	{ &PORTA, &TRISA, 2 },	// SWITCH2
	{ &PORTC, &TRISC, 1 },	// SWITCH3
	{ &PORTB, &TRISB, 3 },	// SWITCH4
	{ &PORTC, &TRISC, 3 },	// KEY1
	{ &PORTC, &TRISC, 5 },	// KEY2
	{ &PORTC, &TRISC, 7 }	// KEY3
},

{
	{ &PORTA, &TRISA, 0 },  // Unused
	{ &PORTA, &TRISA, 1 },	// CAN_RATE
	{ &PORTB, &TRISB, 1 },	// CAN_EN
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 8 },	// SWITCH1
	{ &PORTA, &TRISA, 2 },	// SWITCH2
	{ &PORTC, &TRISC, 1 },	// SWITCH3
	{ &PORTB, &TRISB, 3 },	// SWITCH4
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 },	// Unused
	{ &PORTA, &TRISA, 0 }	// Unused
}};

const unsigned short hw_NoKeys[hw_NoVariants] = { 0, 3 };

unsigned int hw_ReadPort(enum hw_Ports_e port) {
	return ( *( hw_Port[hw_Type][port].port ) >> hw_Port[hw_Type][port].bit ) & 1;
}

void hw_OutputPort(enum hw_Ports_e port) {
	*( hw_Port[hw_Type][port].tris ) &= ~( 1 << hw_Port[hw_Type][port].bit );
}

void hw_InputPort(enum hw_Ports_e port) {
	*( hw_Port[hw_Type][port].port ) |= ( 1 << hw_Port[hw_Type][port].bit );
}

void hw_WritePort(enum hw_Ports_e port, int value) {
	NOP;
	if( value )
		*( hw_Port[hw_Type][port].port ) |= ( 1 << hw_Port[hw_Type][port].bit );
	else
		*( hw_Port[hw_Type][port].port ) &= ~( 1 << hw_Port[hw_Type][port].bit );
	NOP;
}

void hw_InitializeOutputSwitches( char mask ) {
	if( mask & 0x08 ) {
		hw_OutputPort( hw_SWITCH4 );
		hw_WritePort( hw_SWITCH4, 0 );
	}
	if( mask & 0x04 ) {
		hw_OutputPort( hw_SWITCH3 );
		hw_WritePort( hw_SWITCH3, 0 );
	}
	if( mask & 0x02 ) {
		hw_OutputPort( hw_SWITCH2 );
		hw_WritePort( hw_SWITCH2, 0 );
	}
	if( mask & 0x01 ) {
		hw_OutputPort( hw_SWITCH1 );
		hw_WritePort( hw_SWITCH1, 0 );
	}
}
//-------------------------------------------------------------------------------

void hw_Initialize(void) {
	DWORD_VAL fidc, fidc_data;
	char hw_Mask;

	CLKDIVbits.DOZE = 0; // To make fCY = fOSC/2

	AD1PCFGL = 0x1FFF;	// ANx eats ECAN1 SNAFU!
	PORTA = 0;			// ...and setting AD1PCFGL messes with ports!
	PORTB = 0;
	PORTC = 0;

	if( RCONbits.POR ) RCON = 0; // If Power-On-Reset, clear RCON
	RCONbits.SWDTEN = 0; // No Watchdog if FWDTEN set to User Software.
	RCONbits.VREGS = 1; // Keep Voltage regulator active during sleep.

	// Set up clock oscillator. Nominal fOSC=7.37MHz
	// This is too low for the ECAN unit to work well at 250kBit.
	// So we use PLL to increase it to 14MHz.

	_PLLPRE = 2; // Prescale = PLLPRE+2=4 > fRef = 7.37/4=1.84MHz
	_PLLDIV = 59; // Multiply = PLLDIV+2=61 > fVCO = 1.84x61 = 112.4MHz
	_PLLPOST = 3; // Postscale = 8 > fOSC = 112.4/8 = 14.05MHz

	// Resulting fCY = 7024531 Hz nominal

	while( OSCCONbits.LOCK != 1 )
		; // Wait for PLL to lock

	// Find out what hardware we are using.
	// Currently we have two versions: Lamp and Switch/Controller.
	// The 4 byte unique device ID (Microchip refers to this as Unit ID)
	// is used to store all H/W config info.

	// Byte 0 is our device ID in the system.

	fidc.Val = 0xf80016;
	TBLPAG = fidc.word.HW;

	fidc_data.word.HW = __builtin_tblrdh( fidc.word.LW );
	fidc_data.word.LW = __builtin_tblrdl( fidc.word.LW );

	hw_DeviceID = fidc_data.byte.LB;

	// If ID memory was not programmed we have no clue what type of hardware we are.

	if( hw_DeviceID == 0xFF ) {

		// First turn off LED of hw_LED_LAMP.

		_TRISC4 = 0;
		_TRISC5 = 0;
		_RC4 = 1;
		_RC5 = 1;

		// Enable keypad back-light of hw_KEYPAD.

		_TRISB5 = 0;

		// Now hang flashing some lights...

		while( 1 ) {
			_RC4 = 1;
			_RB5 = 1;
			__delay32( 1500000 );
			_RC4 = 0;
			_RB5 = 0;
			__delay32( 20000000 );
		}
	}

	// Find out additional individual config parameters from Unit ID byte 1.

	fidc.Val = 0xf80014;
	fidc_data.word.HW = __builtin_tblrdh( fidc.word.LW );
	fidc_data.word.LW = __builtin_tblrdl( fidc.word.LW );

	hw_ConfigByte = fidc_data.byte.LB;

	hw_I2C_Installed = ( ( hw_ConfigByte & 0x10 ) != 0 );
	hw_Photodetector_Installed = ( ( hw_ConfigByte & 0x20 ) != 0 ); // XXX Fix bug where unit hangs in ADC if disabled!
	hw_Joystick_Installed = ( ( hw_ConfigByte & 0x40 ) != 0 );
	hw_Actuators_Installed = ( ( hw_ConfigByte & 0x80 ) != 0 );
	hw_TankSender_Installed = ( ( hw_ConfigByte & 0x04 ) != 0 );
	hw_Ballast_Cntrl = ( ( hw_ConfigByte & 0x02 ) != 0 );

	// Byte 2 is the type of circuit board we are on.

	fidc.Val = 0xf80012;
	fidc_data.word.HW = __builtin_tblrdh( fidc.word.LW );
	fidc_data.word.LW = __builtin_tblrdl( fidc.word.LW );

	hw_Type = fidc_data.byte.LB;

	fidc.Val = 0xf80010;
	fidc_data.word.HW = __builtin_tblrdh( fidc.word.LW );
	fidc_data.word.LW = __builtin_tblrdl( fidc.word.LW );

	hw_Mask = fidc_data.byte.LB;

	// Check configuration area, erase it if it seems corrupted.

	hw_Config = (hw_Config_t*) &hw_ConfigData;

	if( hw_Config->MagicWord != hw_CONFIG_MAGIC_WORD ) {

		hw_Config = (hw_Config_t*) &hw_1kBuffer;

		hw_Config->MagicWord = hw_CONFIG_MAGIC_WORD;
		hw_Config->nmeaIndustryGroup = nmea_INDUSTRY_GROUP;
		hw_Config->nmeaVehicleSystem = nmea_VEHICLE_SYSTEM;
		hw_Config->nmeaFunction = nmea_FUNCTION_SWITCH;
		hw_Config->nmeaArbitraryAddress = hw_DeviceID;
		hw_Config->nmeaManufacturerCode = nmea_MANUFACTURER_CODE;
		hw_Config->nmeaIdentityNumber = hw_DeviceID;

		// Load some sensible values if we have lost calibrations.

		hw_Config->engine_Calibration[p_ThrottleMin] = 159;
		hw_Config->engine_Calibration[p_ThrottleMax] = 133;
		hw_Config->engine_Calibration[p_GearNeutral] = 151;
		hw_Config->engine_Calibration[p_GearReverse] = 185;
		hw_Config->engine_Calibration[p_GearForward] = 127;

		hw_Config->engine_Calibration[p_JoystickMin] = 100;
		hw_Config->engine_Calibration[p_JoystickMid] = 390;
		hw_Config->engine_Calibration[p_JoystickMax] = 660;
		hw_Config->engine_Calibration[p_ActuatorsTimeout] = schedule_SECOND * 5;

		hw_Config->led_BacklightMultiplier = 2;
		hw_Config->led_BacklightOffset = 10;
		hw_Config->led_BacklightDaylightCutoff = 220;
		hw_Config->led_MinimumDimmedLevel = 7;

		hw_WriteSettingsFlash();

		hw_Config = (hw_Config_t*) &hw_ConfigData; // Set pointer back to Flash data.
	}

	// IO setup for SN65HVD234 CANBus driver.
	// The wiring is different on different versions of the hardware.

	// hw_Type					hw_LEDLAMP		hw_KEYPAD
	//--------------------------------------------------------------
	// Slew Rate control pin	Pin  8 RB10		Pin 20 RA1
	// Driver Enable			Pin  9 RB11		Pin 22 RB1
	// CAN Tx					Pin 10 RB12		Pin 10 RB12
	// CAN Rx					Pin 11 RB13		Pin 21 RB0

	hw_OutputPort( hw_CAN_EN );
	hw_OutputPort( hw_CAN_RATE );

	hw_WritePort( hw_CAN_EN, 0 ); // Chip Enable = 0, go off bus.
	hw_WritePort( hw_CAN_RATE, 0 ); // RS = 0 -> Not in sleep mode.

	// Peripheral mappings

	switch( hw_Type ) {

		case hw_LEDLAMP: {

			hw_DetectorADCChannel = 8;
			if( hw_Photodetector_Installed ) AD1PCFGLbits.PCFG8 = 0;

			hw_OutputPort( hw_LED_RED );
			hw_OutputPort( hw_LED_WHITE );

			PPSUnLock;
			PPSOutput( PPS_OC1, PPS_RP20 ); // Red PWM to RP20.
			PPSOutput( PPS_OC2, PPS_RP21 ); // White PWM to RP21.
			RPOR6bits.RP12R = 0x10; // CAN Transmit to RP12.
			RPINR26bits.C1RXR = 13; // CAN Receive from pin RP13.
			PPSLock;
			break;
		}

		case hw_KEYPAD: {

			hw_DetectorADCChannel = 0;
			if( hw_Photodetector_Installed ) AD1PCFGLbits.PCFG0 = 0;
			if( hw_Joystick_Installed ) AD1PCFGLbits.PCFG10 = 0;

			hw_OutputPort( hw_LED_RED );
			hw_OutputPort( hw_LED1 );
			hw_OutputPort( hw_LED2 );
			hw_OutputPort( hw_LED3 );

			hw_WritePort( hw_LED1, 0 );
			hw_WritePort( hw_LED2, 0 );
			hw_WritePort( hw_LED3, 0 );

			hw_InitializeOutputSwitches( hw_Mask );

			// We don't configure the General Purpose Outputs here, since they
			// are seldom used and we want to minimize current draw.
			// They get configured on first use instead.

			PPSUnLock;
			PPSOutput( PPS_OC1, PPS_RP5 ); // Red Backlight PWM to pin 41.
			hw_PWMInverted = 1;
			RPOR6bits.RP12R = 0x10; // CAN Transmit to pin 10.
			RPINR26bits.C1RXR = 0; // CAN Receive from pin 21.

			if( hw_Actuators_Installed ) {
				PPSOutput( PPS_OC3, PPS_RP2 ); // Ch1: Throttle to pin 23.
				PPSOutput( PPS_OC4, PPS_RP18 ); // Ch1: Gear box to pin 27.
				hw_OutputPort( hw_SWITCH3 );
				hw_WritePort( hw_SWITCH3, 0 );
			}

			if( hw_Joystick_Installed || hw_TankSender_Installed ) {
				hw_OutputPort( hw_SWITCH1 );
			}

			PPSLock;
			break;
		}

		case hw_SWITCH: {

			hw_InitializeOutputSwitches( hw_Mask );

			if( hw_Joystick_Installed || hw_TankSender_Installed ) {
				AD1PCFGLbits.PCFG10 = 0;
				hw_DetectorADCChannel = 0;
			}

			PPSUnLock;
			RPOR6bits.RP12R = 0x10; // CAN Transmit to pin 10.
			RPINR26bits.C1RXR = 0; // CAN Receive from pin 21.

			PPSLock;
			break;

		}
	}

	hw_HeartbeatCounter = 0;

	// Disable unused hardware. Keep Timers, Output Capture, ECAN, ADC and I2C running.

	PMD1 = 0x0078; // Disable UART1, UART2, SPI1, SPI2

	if( !hw_I2C_Installed ) PMD1bits.I2C1MD = 1;

	if(
		(!hw_Photodetector_Installed) &&
		(!hw_Joystick_Installed) &&
		(!hw_TankSender_Installed)
	) PMD1bits.AD1MD = 1;

	PMD2 = 0xC300; // Disable all Input Captures.

	PMD3bits.RTCCMD = 1;
	PMD3bits.CRCMD = 1;
	PMD3bits.PMPMD = 1;

	// Finally, don't fall asleep the first thing we do. Need a few seconds for boot sequence.

	hw_StayAwakeTimer = schedule_SECOND * 5;

	return;
}

//-------------------------------------------------------------------------------
// Copy parameters from Flash to RAM.
// Not that if hw_CONFIG_SIZE exceeds 64 more rows will need to be read.

void hw_ReadConfigFlash() {
	_init_prog_address( hw_ConfigPtr, hw_ConfigData);
	_memcpy_p2d16( &hw_1kBuffer, hw_ConfigPtr, _FLASH_ROW );

	hw_Config = (hw_Config_t*) hw_1kBuffer;
}

//-------------------------------------------------------------------------------
// Copy parameters from RAM to Flash.
// Note that only one row is written although the whole page is erased.

void hw_WriteSettingsFlash() {
	_init_prog_address( hw_ConfigPtr, hw_ConfigData);
	_erase_flash( hw_ConfigPtr );
	_write_flash16( hw_ConfigPtr, (int*) hw_1kBuffer );
}

//-------------------------------------------------------------------------------

unsigned char hw_IsLED(unsigned short hw_Port) {

	if( hw_Port == hw_LED_RED || hw_Port == hw_LED_WHITE || hw_Port == hw_LED_LIGHT || hw_Port == hw_BACKLIGHT ) return 1;

	return 0;
}

//-------------------------------------------------------------------------------

unsigned char hw_IsActuator(unsigned short hw_Port) {

	if( hw_Port == hw_PWM1 || hw_Port == hw_PWM2 ) return 1;

	return 0;
}

//-------------------------------------------------------------------------------

unsigned char hw_IsSwitch(unsigned short hw_Port) {

	if( hw_Port >= hw_SWITCH1 && hw_Port <= hw_SWITCH4 ) return 1;

	return 0;
}

//---------------------------------------------------------------------------------------------

void hw_AcknowledgeSwitch(unsigned char port, int setting) {

	switch( port ) {
		case hw_KEY1: {
			hw_WritePort( hw_LED1, setting );
			hw_LEDStatus = hw_LEDStatus & 6;
			hw_LEDStatus = hw_LEDStatus | setting;
			break;
		}
		case hw_KEY2: {
			hw_WritePort( hw_LED2, setting );
			hw_LEDStatus = hw_LEDStatus & 5;
			hw_LEDStatus = hw_LEDStatus | setting << 1;
			break;
		}
		case hw_KEY3: {
			hw_WritePort( hw_LED3, setting );
			hw_LEDStatus = hw_LEDStatus & 3;
			hw_LEDStatus = hw_LEDStatus | setting << 2;
			break;
		}
	}
}

//-------------------------------------------------------------------------------

void hw_Sleep(void) {

	// Check for things that need us to stay awake:

	if( hw_StayAwakeTimer > 0 ) return;
	if( !queue_Empty( events_Queue ) ) return;
	if( nmea_TX_REQUEST_BIT ) return;
	if( (nmea_TxQueueTail != nmea_TxQueueHead) || nmea_TxQueueFull ) return;
	if( display_IsOn ) return;
	if( engine_CurMasterDevice == hw_DeviceID ) return;
	if( engine_CurMasterDevice && hw_Actuators_Installed ) return;
	if( schedule_HaveSleepingTask ) return;

	// OK, We can sleep now.

	//hw_WritePort( hw_LED1, 1 );
	hw_WritePort( hw_CAN_RATE, 1 );
	nmea_ControllerMode( hw_ECAN_MODE_DISABLE );

	if( hw_CPUStopPossible ) {
		// Deep sleep, clocks stopped.
		asm volatile ("PWRSAV #0");
	}
	else {
		// Idle with PWM clocks still running.
		asm volatile ("PWRSAV #1");
	}

	nmea_ControllerMode( hw_ECAN_MODE_NORMAL );
	hw_WritePort( hw_CAN_RATE, 0 );
	//hw_WritePort( hw_LED1, 0 );
}

//-------------------------------------------------------------------------------

void ADC_Initialize(void) {

	AD1CON1bits.ADON = 0;

	AD1CON1 = 0x00E0; // Idle=Stop, 10bit, unsigned, Auto conversion.
	AD1CON2bits.VCFG = 0x0; // AVss/AVdd
	AD1CON2bits.CHPS = 0x0; // 1 Channel conversion.
	AD1CON2bits.ALTS = 0x0; // Only use MUX A.
	AD1CON3 = 0x0800; // System clock, Ts = 8xTad, Tad=1xTcy.

	AD1CSSL = 0x0000; // No scanning.
	IFS0bits.AD1IF = 0;

}

//-------------------------------------------------------------------------------

unsigned int ADC_Read(unsigned char channel) {

	AD1CHS0bits.CH0SA = channel; // Channel 0 MUX A input.

	AD1CON1bits.ADON = 1;

	AD1CON1bits.SAMP = 1;
	while( !AD1CON1bits.DONE )
		;

	AD1CON1bits.ADON = 0;

	return ADC1BUF0;
}
