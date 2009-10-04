#include "hw.h"

#include <i2c.h>
#include <stdio.h>

#include "display.h"


unsigned char display_PendingKeypress = 0;
unsigned char display_IsOn = 1;
unsigned char display_CurrentAdjust = 0;
short display_Brightness = 0xFF;
short display_Contrast = 0x80;


void display_Keypress( unsigned char key ) {
/*	char line1[20];
	display_SetPosition(1,1);
	sprintf( line1, "Key: 0x%02X", key );
	display_Write( line1 );
*/
	switch( key ) {

		case DISPLAY_KEY_ONOFF:		{
					if( display_IsOn ) {
						display_Off();
						display_IsOn = 0;
					}
					else {
						display_On();
						display_IsOn = 1;
					}
					break;
				}

		case DISPLAY_KEY_F1: {
					display_CurrentAdjust = ! display_CurrentAdjust;
					break;
				}
		case DISPLAY_KEY_F2:		{ break; }
		case DISPLAY_KEY_F3:		{ break; }

		case DISPLAY_KEY_UP: { 
					if( display_CurrentAdjust ) {
						display_Brightness += 0x10 ;
						if( display_Brightness > 0xFF ) display_Brightness = 0xFF;
						display_SetBrightness( display_Brightness );
					} else {
						display_Contrast += 0x10 ;
						if( display_Contrast > 0xFF ) display_Contrast = 0xFF;
						display_SetContrast( display_Contrast );
					}
					break;
				}

		case DISPLAY_KEY_DOWN: {
					if( display_CurrentAdjust ) {
						display_Brightness -= 0x10 ;
						if( display_Brightness < 0 ) display_Brightness = 0;
						display_SetBrightness( display_Brightness );
					} else {
						display_Contrast -= 0x10 ;
						if( display_Contrast < 0 ) display_Contrast = 0;
						display_SetContrast( display_Contrast );
					}
					break;
				}

		case DISPLAY_KEY_LEFT:		{ break; }
		case DISPLAY_KEY_STOP:		{ break; }
		case DISPLAY_KEY_FWD:		{ break; }
		case DISPLAY_KEY_RIGHT:		{ break; }
		case DISPLAY_KEY_NORTH:		{ break; }
		case DISPLAY_KEY_SOUTH:		{ break; }
		case DISPLAY_KEY_EAST:		{ break; }
		case DISPLAY_KEY_WEST:		{ break; }
		case DISPLAY_KEY_E:			{ break; }
	}
}

void display_Initialize() {
	unsigned int config2, config1;
	config2 = 0x50;

	config1 = (I2C1_ON & I2C1_IDLE_CON & I2C1_CLK_HLD &
	           I2C1_IPMI_DIS & I2C1_7BIT_ADD &
	           I2C1_SLW_DIS & I2C1_SM_DIS &
	           I2C1_GCALL_DIS & I2C1_STR_DIS &
	           I2C1_NACK & I2C1_ACK_DIS & I2C1_RCV_DIS &
	           I2C1_STOP_DIS & I2C1_RESTART_DIS &
	           I2C1_START_DIS);
	OpenI2C1(config1,config2);


	display_Send3bytes( DISPLAY_CMD, DISPLAY_RS232_AUTOXMIT, 0 );
	display_Send3bytes( DISPLAY_CMD, DISPLAY_DEBOUNCE, 12 );
	display_Send2bytes( DISPLAY_CMD, DISPLAY_WRAP_ON );
	for( config1=0; config1<1000; config1++) __delay32(11);
	display_SetBrightness( display_Brightness );
	for( config1=0; config1<1000; config1++) __delay32(11);
	display_SetContrast( display_Contrast );
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

unsigned char display_Write( char *str ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	MasterputsI2C1( (unsigned char*)str );
	IdleI2C1();
	StopI2C1();
	return status;
}

unsigned char display_Sendbyte( unsigned char byte ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	MasterWriteI2C1( byte );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

unsigned char display_Send2bytes( unsigned char b1, unsigned char b2 ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	MasterWriteI2C1( b1 );
	IdleI2C1();
	MasterWriteI2C1( b2 );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

unsigned char display_Send3bytes( unsigned char b1, unsigned char b2, unsigned char b3 ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	MasterWriteI2C1( b1 );
	IdleI2C1();
	MasterWriteI2C1( b2 );
	IdleI2C1();
	MasterWriteI2C1( b3 );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

unsigned char display_Send4bytes( unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4 ) {
	unsigned char status;
	status = display_Address(0);
	if( status != 0 ) return status;
	status = MasterWriteI2C1( b1 );
	if( status != 0 ) return status;
	IdleI2C1();
	status = MasterWriteI2C1( b2 );
	if( status != 0 ) return status;
	IdleI2C1();
	status = MasterWriteI2C1( b3 );
	if( status != 0 ) return status;
	IdleI2C1();
	status = MasterWriteI2C1( b4 );
	if( status != 0 ) return status;
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
	return status;
}

void display_Clear() {
	display_Send2bytes( DISPLAY_CMD, DISPLAY_CLEAR );
}

void display_Home() {
	display_Send2bytes( DISPLAY_CMD, DISPLAY_HOME );
}

void display_On() {
	unsigned char status = 0;
	do {
		if( status != 0 ) {
			int i;
			for( i=0; i<1000; i++ )
				__delay32(11);
		}
		status = display_Send3bytes( DISPLAY_CMD, DISPLAY_ON, 0 );
	}
	while( status != 0 );
}

void display_Off() {
	unsigned char status = 0;
	do {
		if( status != 0 ) {
			int i;
			for( i=0; i<1000; i++ )
				__delay32(11);
		}
		status = display_Send2bytes( DISPLAY_CMD, DISPLAY_OFF );
	}
	while( status != 0 );
}

void display_SetPosition( unsigned char column, unsigned char row ) {
	unsigned char status = 0;
	if( column <  1 ) column = 1;
	if( column > 20 ) column = 20;
	if( row < 1 ) row = 1;
	if( row > 4 ) row = 4;

	do {
		if( status != 0 ) { 
			int i; 
			for( i=0; i<1000; i++ ) 
				__delay32(11);
		}
		status = display_Send4bytes( DISPLAY_CMD, DISPLAY_SETPOS, column, row );
	}
	while( status != 0 );
}

void display_SetBrightness( unsigned char value ) {
	unsigned char status = 0;
	do {
		if( status != 0 ) { 
			int i; 
			for( i=0; i<1000; i++ ) 
				__delay32(11);
		}
		status = display_Send3bytes( DISPLAY_CMD, DISPLAY_BRIGHTNESS, value );
	}
	while( status != 0 );
}

void display_SetContrast( unsigned char value ) {
	unsigned char status = 0;
	do {
		if( status != 0 ) { 
			int i; 
			for( i=0; i<1000; i++ ) 
				__delay32(11);
		}
		status = display_Send3bytes( DISPLAY_CMD, DISPLAY_CONTRAST, value );
	}
	while( status != 0 );
}

unsigned char display_ReadKeypad() {
	unsigned char status;
	status = display_Address(1);
	if( status != 0 ) return status;
	return MasterReadI2C1();
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
}
