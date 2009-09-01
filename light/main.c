#define FCY 698800UL

#include <p24hxxxx.h>
#include <pps.h>
#include <libpic30.h>
#include <limits.h>

#include "hw.h"
#include "led.h"
#include "nmea.h"
#include "events.h"
#include "ctrlkey.h"

void goodnight( void );

_FOSCSEL( FNOSC_FRCPLL )
_FWDT( FWDTEN_OFF )
_FICD( ICS_PGD1 )

int main (void)
{
	long lastTimer, thisTimer, interval;
	unsigned char currentColor = led_RED,
			holding = 0;
	float lastLevel, newLevel, fadeStep;

	hw_Initialize();

	TRISBbits.TRISB14 = 0; // Debug

	led_Initialize();

	led_PresetLevel[ led_RED ] = 1.0;
	led_PresetLevel[ led_WHITE ] = 0.5;

	led_FadeToLevel( led_RED, 0.0, 2.0 );
	led_FadeToLevel( led_WHITE, 0.0, 2.0 );

	events_Initialize();

	nmea_Initialize();

	ctrlkey_Initialize();

	// Main event loop. Send NMEA events on the bus if keys clicked,
	// and respond to incoming commands.
	// The MAINTAIN_POWER PGN should be sent before any command, to ensure
	// all sleeping units wake up before the real data arrives.

	while (1) {

		_RB14 = 0;
		//if( led_SleepTimer > 250 ) goodnight();

		eventPtr = events_Pop();
		if( eventPtr ) {
			switch( eventPtr->type ) {
				case event_KEY_CLICKED: {
					led_SleepTimer = 0;
					nmea_MakePGN( &outPGN, 0, nmea_MAINTAIN_POWER, 0 );
					nmea_SendMessage( &outPGN );
					nmea_MakePGN( &outPGN, 0, nmea_LIGHTING_COMMAND, 4 );
					outPGN.data[0] = eventPtr->type;
					outPGN.data[1] = eventPtr->data;
					outPGN.data[2] = (eventPtr->atTimer&0xFF00) >> 8;
					outPGN.data[3] = eventPtr->atTimer&0x00FF;
					nmea_SendMessage( &outPGN );
					break;
				}
				case event_KEY_HOLDING: {
					led_SleepTimer = 0;
					nmea_MakePGN( &outPGN, 0, nmea_MAINTAIN_POWER, 0 );
					nmea_SendMessage( &outPGN );
					nmea_MakePGN( &outPGN, 0, nmea_LIGHTING_COMMAND, 4 );
					outPGN.data[0] = eventPtr->type;
					outPGN.data[1] = eventPtr->data;
					outPGN.data[2] = (eventPtr->atTimer&0xFF00) >> 8;
					outPGN.data[3] = eventPtr->atTimer&0x00FF;
					nmea_SendMessage( &outPGN );
					break;
				}
				case event_KEY_RELEASED: {
					led_SleepTimer = 0;
					nmea_MakePGN( &outPGN, 0, nmea_MAINTAIN_POWER, 0 );
					nmea_SendMessage( &outPGN );
					nmea_MakePGN( &outPGN, 0, nmea_LIGHTING_COMMAND, 4 );
					outPGN.data[0] = eventPtr->type;
					outPGN.data[1] = eventPtr->data;
					outPGN.data[2] = (eventPtr->atTimer&0xFF00) >> 8;
					outPGN.data[3] = eventPtr->atTimer&0x00FF;
					nmea_SendMessage( &outPGN );
					break;
				}
				case event_WDT_RESET: {
					if( holding ) {
						newLevel = led_CurrentLevel[currentColor];
						newLevel += fadeStep;
						if( newLevel > 1.0 ) { fadeStep = - fadeStep; newLevel = 1.0; }
						if( newLevel < 0.0 ) { fadeStep = - fadeStep; newLevel = 0.0; }
						led_SetLevel( currentColor, newLevel );
					}
					break;
				}
				case event_NMEA_MESSAGE: {
					led_SleepTimer = 0;
					nmea_GetReceivedPGN( &inPGN );
					if( inPGN.PGN == nmea_LIGHTING_COMMAND ) {
						lastTimer = thisTimer;
						thisTimer = 256*inPGN.data[2] + inPGN.data[3];
						interval = thisTimer - lastTimer;
						if( interval < 0 ) interval += USHRT_MAX;

						switch( inPGN.data[0] ) {
							case event_KEY_HOLDING: {
								holding = 1;
								if( led_CurrentLevel[currentColor] > 0.5 ) fadeStep = -0.1;
								else fadeStep = 0.1;
								break;
							}
							case event_KEY_RELEASED: {
								holding = 0;
								break;
							}
							case event_KEY_CLICKED: {

								// Double or single click?

								if( interval < 400 ) {
									led_StopFade( currentColor );
									led_SetLevel( currentColor, lastLevel );
									if( currentColor == led_RED ) currentColor = led_WHITE;
									else currentColor = led_RED;
								} else {
									lastLevel = led_CurrentLevel[currentColor];
									led_Toggle( currentColor );
								}
								break;
							}
						}
					}
					break;
				}
			}
		}
	}
}


void goodnight( void ) {
	
	_RB14 = 1;
	nmea_ControllerMode( 1 ); // Disable

	if( led_CanSleep ) {
		asm volatile ("PWRSAV #0");
	}
	else { // Idle with PWM clocks still running. 
		asm volatile ("PWRSAV #1");
	}

	_RB14 = 0;
	led_SleepTimer = 0;
	nmea_ControllerMode( 0 ); // Normal Operation
}
