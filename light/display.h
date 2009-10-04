#ifndef __display_H
#define __display_H

#define DISPLAY_I2C_ADDR		0x50
#define DISPLAY_CMD				254
#define DISPLAY_CLEAR			88
#define DISPLAY_HOME			72
#define DISPLAY_SETPOS			71
#define DISPLAY_WRAP_ON			67
#define DISPLAY_WRAP_OFF		68
#define DISPLAY_ON				66
#define DISPLAY_OFF				70
#define DISPLAY_DEBOUNCE		85
#define DISPLAY_BRIGHTNESS		152
#define DISPLAY_CONTRAST		80
#define DISPLAY_RS232_AUTOXMIT	160

void display_Write( unsigned char *str );
void display_Sendbyte( unsigned char byte );
void display_Send2bytes( unsigned char b1, unsigned char b2 );
void display_Send3bytes( unsigned char b1, unsigned char b2, unsigned char b3 );
void display_Send4bytes( unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4 );
void display_Initialize();
void display_Clear();
void display_Home();
void display_On();
void display_Off();
void display_SetPosition( unsigned char column, unsigned char row );
void display_Brightness( unsigned char value );
void display_Contrast( unsigned char value );
unsigned char display_ReadKeypad();

#endif
