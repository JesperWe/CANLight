#include "hw.h"

#include <i2c.h>
#include <stdio.h>
#include <stdarg.h>

#include "display.h"
#include "config.h"
#include "events.h"
#include "nmea.h"
#include "led.h"
#include "menu.h"
#include "schedule.h"

unsigned char display_IsOn;
unsigned char display_CurrentAdjust = 0;
short display_Brightness = 0xFF;
short display_Contrast = 0x80;
unsigned short display_TankLevels[4];
queue_t* display_Queue = 0;

void display_Initialize() {
	unsigned int config2, config1;
	unsigned char status;

	config2 = 0x50;

	config1 = (I2C1_ON & I2C1_IDLE_CON & I2C1_CLK_HLD &
	           I2C1_IPMI_DIS & I2C1_7BIT_ADD &
	           I2C1_SLW_DIS & I2C1_SM_DIS &
	           I2C1_GCALL_DIS & I2C1_STR_DIS &
	           I2C1_NACK & I2C1_ACK_DIS & I2C1_RCV_DIS &
	           I2C1_STOP_DIS & I2C1_RESTART_DIS &
	           I2C1_START_DIS);
	OpenI2C1(config1,config2);

	status = display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_RS232_AUTOXMIT, 0 );

	if( status & 0x80 ) {
		// Addressing failed. Display is not connected or maybe powered off.
		CloseI2C1();
		display_IsOn = FALSE;
		return;
	}

	display_IsOn = TRUE;

	display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_DEBOUNCE, 12 );
	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_WRAP_ON );
	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_AUTOSCROLL_ON );
	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_FLUSH_KEYS );

	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_INIT_HBAR );

	display_SetBrightness( display_Brightness );

	display_SetContrast( display_Contrast );

	if( display_Queue == 0 ) display_Queue = queue_Create( 5, 2 );

/* Code below only needed when initializing a factory reset display.
   Maybe we should set up a RS232 connection to do this is the future?
	do {
		status = display_Address(0);
		if( status != 0 ) continue;
		status = MasterWriteI2C1( DISPLAY_CMD );
		IdleI2C1();
		if( status != 0 ) continue;
		status = MasterWriteI2C1( 0x40 );
		IdleI2C1();
		if( status != 0 ) continue;
	} while( status != 0 );
	//               1234567890123456789||234567890123456789||234567890123456789||2345678901234567890
	MasterputsI2C1( "                         JOURNEYMAN        CONTROL SYSTEM                       " );
	IdleI2C1();
	StopI2C1();
	IdleI2C1(); */
	
	return;
}


unsigned char display_Address( unsigned char read ) {
	unsigned char status;

	IdleI2C1();
	StartI2C1();
	IdleI2C1();
	status = MasterWriteI2C1( DISPLAY_I2C_ADDR | read );
	IdleI2C1();
	return status;
}

unsigned char display_Write( const char *str ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	MasterputsI2C1( (unsigned char*)str );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

unsigned char display_Sendbytes( int nBytes, ... ) {
	unsigned char status, byte;
	va_list ap;
	int i;

	va_start( ap, nBytes );

	status = display_Address(0);
	if( status != 0 ) return status;

	for( i=0; i<nBytes; i++ ) {
		byte = va_arg( ap, unsigned char );
		MasterWriteI2C1( byte );
		IdleI2C1();
	}

	va_end( ap );

	StopI2C1();
	IdleI2C1();

	return status;
}


void display_Clear() {
	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_CLEAR );
}

void display_Home() {
	display_Sendbytes( 2, DISPLAY_CMD, DISPLAY_HOME );
}

void display_On() {
	unsigned char status = 0;
	status = display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_ON, 0 );
}

void display_Off() {
	unsigned char status = 0;
	status = display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_OFF );
}

void display_SetPosition( unsigned char column, unsigned char row ) {
	unsigned char status = 0;
	if( column <  1 ) column = 1;
	if( column > 20 ) column = 20;
	if( row < 1 ) row = 1;
	if( row > 4 ) row = 4;

	status = display_Sendbytes( 4, DISPLAY_CMD, DISPLAY_SETPOS, column, row );
}

void display_SetBrightness( unsigned char value ) {
	unsigned char status = 0;
	status = display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_BRIGHTNESS, value );
}

void display_SetContrast( unsigned char value ) {
	unsigned char status = 0;
	status = display_Sendbytes( 3, DISPLAY_CMD, DISPLAY_CONTRAST, value );
}

unsigned char display_ReadKeypad() {
	unsigned char status;
	status = display_Address(1);
	if( status != 0 ) return status;
	status = MasterReadI2C1();
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

void display_HorizontalBar( unsigned char col, unsigned char row, unsigned char value ) {
	if( value > 100 ) value = 100;
	display_Sendbytes( 6, DISPLAY_CMD, DISPLAY_PLACE_HBAR, col, row, 0, value );
}


//---------------------------------------------------------------------------------------------
// Home-grown �ber-simplified formatting function since STDC sprintf() takes about 5k Bytes of code...

void display_NumberFormat( char outString[], short digits, short number ) {

	if( digits == 0 ) {
		digits = 4;
		if( number < 1000 ) digits = 3;
		if( number < 100 ) digits = 2;
		if( number < 10 ) digits = 1;
	}

	outString[ digits ] = 0;
	do {
		outString[digits-1] = '0' + (number % 10);
		number = number / 10;
		digits--;
	} while( digits > 0 );
}

//---------------------------------------------------------------------------------------------
// We can either poll the keypad or read a pressed key in one time slot.
// The display is too slow to poll and process in the same cycle, it will protest.
// So we put read keystrokes in a queue, and process them in another task.

void display_Task() {
	unsigned short key;
	static unsigned char failCount = 0;
	static unsigned char requestCount = 0;

	if( display_IsOn ) {
		key = display_ReadKeypad();

		if( key != 0x00 ) {

			// If MSB is set there was a negative status which means the display
			// did not ACK. It is probably busy. So we try again next time slot.

			if( (key & 0x80) != 0 ) {
				failCount++;
				key = 0;
				if( failCount > 2 ) {
					display_IsOn = FALSE;
				}
			}
			else {
				queue_Send( display_Queue, &key );
			}
		}

		// In this task we also run the tank level request sender, so
		// we get tank levels reported from all attached senders as long as
		// the display is switched on.

		if( requestCount == 0 ) {
			event_t event;
			event.PGN = 0;
			event.groupId = config_GetGroupIdForPort( hw_ANALOG );
			event.ctrlDev = hw_DeviceID;
			event.ctrlPort = hw_ANALOG;
			event.ctrlEvent = e_REQUEST_TANK_LEVELS;
			event.data = 0;
			event.info = 0;
			nmea_Wakeup();
			nmea_SendEvent( &event );
		}

		requestCount = (requestCount + 1) % 15;

		return;
	}

	failCount++;
	if( failCount > 10 ) {
		failCount = 0;
		display_Initialize();
		if( display_IsOn ) menu_Initialize();
	}
}


//---------------------------------------------------------------------------------------------
// Set system back-light level based on ambient light.

void display_BacklightTask() {
	static float ambientLevel;
	static char interval;
	unsigned short pvVoltage;
	event_t	ambientEvent;

	if( ! hw_AutoBacklightMode ) return;

	// Don't do anything if we are shining our own light into the detector.
	if( hw_Type == hw_LEDLAMP && (led_CurrentLevel[0]!=0 || led_CurrentLevel[1]!=0) ) return;

	pvVoltage = ADC_Read( hw_DetectorADCChannel );

	ambientLevel = 0.9*ambientLevel + 0.1*(float)pvVoltage;
	interval++;

	if( interval < 15 ) return;
	interval = 0;

	pvVoltage = ((short)ambientLevel) >> 2;

	if( hw_AmbientLevel != pvVoltage ) {

		hw_AmbientLevel = pvVoltage;

		ambientEvent.PGN = 0;
		ambientEvent.data = 0;
		ambientEvent.groupId = 0;
		ambientEvent.ctrlDev = hw_DeviceID;
		ambientEvent.ctrlPort = hw_BACKLIGHT;
		ambientEvent.ctrlEvent = e_AMBIENT_LIGHT_LEVEL;
		ambientEvent.info = hw_AmbientLevel;

		nmea_Wakeup();
		nmea_SendEvent( &ambientEvent );
	}
}


//---------------------------------------------------------------------------------------------

int display_TankMonitor() {
	display_Clear();
	display_SetPosition( 1, 1 );
	display_Write( "Water P:" );
	display_SetPosition( 1, 2 );
	display_Write( "Water S:" );
	display_SetPosition( 2, 3 );
	display_Write( "Fuel P:" );
	display_SetPosition( 2, 4 );
	display_Write( "Fuel S:" );

	schedule_AddTask( display_TankMonitorUpdater, schedule_SECOND );
	return menu_NO_DISPLAY_UPDATE;
}

void display_TankMonitorUpdater() {
	unsigned char tank, level;

	if( menu_CurStateId != menu_HandlerStateId ) {
		schedule_Finished();
		return;
	}

	// Show a graphical representation of the tank levels.

	for( tank=0; tank<4; tank++ ) {
		level = 1 + display_TankLevels[tank] / 14; // Max level is about 700.
		display_HorizontalBar( 10, tank+1, level );
	}
	return;
}


