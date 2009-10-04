#ifndef __display_H
#define __display_H

#define DISPLAY_I2C_ADDR		0x50
#define DISPLAY_CMD			0xFE
#define DISPLAY_CLEAR			88
#define DISPLAY_HOME			72
#define DISPLAY_SETPOS			71
#define DISPLAY_WRAP_ON		67
#define DISPLAY_WRAP_OFF		68
#define DISPLAY_ON				66
#define DISPLAY_OFF			70
#define DISPLAY_DEBOUNCE		85
#define DISPLAY_BRIGHTNESS		152
#define DISPLAY_CONTRAST		80
#define DISPLAY_RS232_AUTOXMIT	160

#define DISPLAY_KEY_ONOFF		0x49
#define DISPLAY_KEY_F1			0x48
#define DISPLAY_KEY_F2			0x46
#define DISPLAY_KEY_F3			0x47
#define DISPLAY_KEY_UP			0x4E
#define DISPLAY_KEY_DOWN		0x53
#define DISPLAY_KEY_LEFT		0x4D
#define DISPLAY_KEY_STOP		0x52
#define DISPLAY_KEY_FWD			0x4B
#define DISPLAY_KEY_RIGHT		0x50
#define DISPLAY_KEY_NORTH		0x4C
#define DISPLAY_KEY_SOUTH		0x51
#define DISPLAY_KEY_EAST		0x57
#define DISPLAY_KEY_WEST		0x58
#define DISPLAY_KEY_E			0x55

void display_Keypress( unsigned char key );

void display_Initialize();
unsigned char display_Write( char *str );
unsigned char display_Sendbyte( unsigned char byte );
unsigned char display_Send2bytes( unsigned char b1, unsigned char b2 );
unsigned char display_Send3bytes( unsigned char b1, unsigned char b2, unsigned char b3 );
unsigned char display_Send4bytes( unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4 );

void display_Clear();
void display_Home();
void display_On();
void display_Off();
void display_SetPosition( unsigned char column, unsigned char row );
void display_SetBrightness( unsigned char value );
void display_SetContrast( unsigned char value );

unsigned char display_ReadKeypad();

extern unsigned char display_PendingKeypress;

#endif
