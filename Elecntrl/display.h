#ifndef __display_H
#define __display_H

#define DISPLAY_I2C_ADDR		0x50
#define DISPLAY_CMD			0xFE
#define DISPLAY_ON				66
#define DISPLAY_WRAP_ON		67
#define DISPLAY_WRAP_OFF		68
#define DISPLAY_FLUSH_KEYS		69
#define DISPLAY_OFF			70
#define DISPLAY_SETPOS			71
#define DISPLAY_HOME			72
#define DISPLAY_CONTRAST		80
#define DISPLAY_AUTOSCROLL_ON	81
#define DISPLAY_AUTOSCROLL_OFF	82
#define DISPLAY_DEBOUNCE		85
#define DISPLAY_CLEAR			88
#define DISPLAY_INIT_HBAR		104
#define DISPLAY_PLACE_HBAR		124
#define DISPLAY_BRIGHTNESS		152
#define DISPLAY_RS232_AUTOXMIT	160

#define DISPLAY_KEY_UP			0x47
#define DISPLAY_KEY_DOWN		0x46
#define DISPLAY_KEY_LEFT		0x53
#define DISPLAY_KEY_STOP		0x52
#define DISPLAY_KEY_PLAY		0x51
#define DISPLAY_KEY_RIGHT		0x50
#define DISPLAY_KEY_NORTH		0x57
#define DISPLAY_KEY_SOUTH		0x56
#define DISPLAY_KEY_EAST		0x55
#define DISPLAY_KEY_WEST		0x58

#define display_ROWS_HIGH		4
#define display_COLS_WIDE		20

void display_Keypress( unsigned char key );

void display_Initialize();
unsigned char display_Address( unsigned char read );
unsigned char display_Write( const char *str );
unsigned char display_Sendbytes( int nBytes, ... );

void display_Clear();
void display_Home();
void display_On();
void display_Off();
void display_SetPosition( unsigned char column, unsigned char row );
void display_SetBrightness( unsigned char value );
void display_SetContrast( unsigned char value );
void display_HorizontalBar( unsigned char col, unsigned char row, unsigned char value );
void display_NumberFormat( char outString[], short digits, short number );
void display_BacklightTask();
int display_TankMonitor();
void display_TankMonitorUpdater();

unsigned char display_ReadKeypad();
void display_Task();

extern unsigned char display_IsOn;
extern queue_t* display_Queue;
extern unsigned short display_TankLevels[];

#endif
