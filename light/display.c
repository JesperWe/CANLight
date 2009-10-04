#include "hw.h"

#include <i2c.h>
#include "display.h"

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
	display_Send2bytes( DISPLAY_CMD, DISPLAY_WRAP_OFF );
}


void display_Address() {
	IdleI2C1();
	StartI2C1();
	IdleI2C1();
	MasterWriteI2C1( DISPLAY_I2C_ADDR );
	IdleI2C1();
}

void display_Write( unsigned char *str ) {
	display_Address();
	MasterputsI2C1( str );
	IdleI2C1();
	StopI2C1();
}

void display_Sendbyte( unsigned char byte ) {
	display_Address();
	MasterWriteI2C1( byte );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
}

void display_Send2bytes( unsigned char b1, unsigned char b2 ) {
	display_Address();
	MasterWriteI2C1( b1 );
	IdleI2C1();
	MasterWriteI2C1( b2 );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
}

void display_Send3bytes( unsigned char b1, unsigned char b2, unsigned char b3 ) {
	display_Address();
	MasterWriteI2C1( b1 );
	IdleI2C1();
	MasterWriteI2C1( b2 );
	IdleI2C1();
	MasterWriteI2C1( b3 );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
}

void display_Send4bytes( unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4 ) {
	display_Address();
	MasterWriteI2C1( b1 );
	IdleI2C1();
	MasterWriteI2C1( b2 );
	IdleI2C1();
	MasterWriteI2C1( b3 );
	IdleI2C1();
	MasterWriteI2C1( b4 );
	IdleI2C1();
	StopI2C1();
	IdleI2C1();
}

void display_Clear() {
	display_Send2bytes( DISPLAY_CMD, DISPLAY_CLEAR );
}

void display_Home() {
	display_Send2bytes( DISPLAY_CMD, DISPLAY_HOME );
}

void display_On() {
	display_Send3bytes( DISPLAY_CMD, DISPLAY_ON, 0 );
}

void display_Off() {
	display_Send2bytes( DISPLAY_CMD, DISPLAY_OFF );
}

void display_SetPosition( unsigned char column, unsigned char row ) {
	if( column <  1 ) column = 1;
	if( column > 20 ) column = 20;
	if( row < 1 ) row = 1;
	if( row > 4 ) row = 4;
	display_Send4bytes( DISPLAY_CMD, DISPLAY_SETPOS, column, row );
}

void display_Brightness( unsigned char value ) {
	display_Send3bytes( DISPLAY_CMD, DISPLAY_BRIGHTNESS, value );
}

void display_Contrast( unsigned char value ) {
	display_Send3bytes( DISPLAY_CMD, DISPLAY_CONTRAST, value );
}

unsigned char display_ReadKeypad() {
	unsigned char key;
	//key = EECurrentAddRead( DISPLAY_I2C_ADDR | 1 );
}
