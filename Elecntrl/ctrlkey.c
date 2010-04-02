/*
 * ctrlkey.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper We
 */

#include "hw.h"
#include "config.h"
#include "config_groups.h"
#include "events.h"
#include "ctrlkey.h"
#include "engine.h"
#include "schedule.h"

//---------------------------------------------------------------------------------------------
// Globals

unsigned char ctrlkey_KeyState[ctrlkey_MAX_NO_KEYS];
unsigned short ctrlkey_NoKeys;
unsigned char ctrlkey_Holding[ ctrlkey_MAX_NO_KEYS ];
unsigned long ctrlkey_Presstime[ ctrlkey_MAX_NO_KEYS ];
unsigned long ctrlkey_Releasetime[ ctrlkey_MAX_NO_KEYS ];
unsigned char ctrlkey_ClickCount[ ctrlkey_MAX_NO_KEYS ];

static const unsigned char ctrlkey_Key2Function[] = { hw_KEY1, hw_KEY2, hw_KEY3 };


//---------------------------------------------------------------------------------------------

void ctrlkey_Initialize( void ) {

	switch( hw_Type ) {

		case hw_LEDLAMP: {
			ctrlkey_NoKeys = 0;
			break;;
		}

		case hw_SWITCH:	 {
			ctrlkey_NoKeys = 3;
			hw_InputPort( hw_KEY1 );
			hw_InputPort( hw_KEY2 );
			hw_InputPort( hw_KEY3 );

			// Setup Change Notification Interrupts.

			CNEN2bits.CN28IE = 1;	// RC3
			CNEN2bits.CN26IE = 1;	// RC5
			CNEN2bits.CN17IE = 1;	// RC7
			IEC1bits.CNIE = 1;
			_CNIF = 0;
			break;
		}

		default:			ctrlkey_NoKeys = 0; return;
	}
}

//---------------------------------------------------------------------------------------------
// Read all defined keys into a bit mask.

unsigned short ctrlkey_ReadKeys( void ) {
	unsigned short newstate;

	newstate = 0;

	if( hw_ReadPort( hw_KEY1 ) == 0 ) newstate |= 1;
	if( hw_ReadPort( hw_KEY2 ) == 0 ) newstate |= 2;
	if( hw_ReadPort( hw_KEY3 ) == 0 ) newstate |= 4;

	return newstate;
}


//---------------------------------------------------------------------------------------------
// The Control Key Task monitors time elapsed between key clicks and determines
// if the click is a single/double/tripple click, and sends bus events accordingly.

void ctrlkey_task() {
	unsigned char keyNo;
	unsigned char event;

	_CNIE = 0; // Disable interrupts while we process, otherwise confusion might ensue...

	for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {

		if( ctrlkey_Holding[ keyNo ] ) continue;

		if( ctrlkey_Releasetime[ keyNo ] ) {
			if( (schedule_time - ctrlkey_Releasetime[ keyNo ]) > ctrlkey_DOUBLECLICK_THRESHOLD ) {
				switch( ctrlkey_ClickCount[ keyNo ] ) {
				case 2: { event = e_KEY_DOUBLECLICKED; break; }
				case 3: { event = e_KEY_TRIPLECLICKED; break; }
				default: { event = e_KEY_CLICKED; break; }
				}

				events_Push( event, 0, hw_DeviceID,
						ctrlkey_Key2Function[ keyNo ],
						event, keyNo, (short)ctrlkey_Presstime[ keyNo ] );

				ctrlkey_ClickCount[ keyNo ] = 0;
				ctrlkey_Presstime[ keyNo ] = 0;
				ctrlkey_Releasetime[ keyNo ] = 0;
				ctrlkey_Holding[ keyNo ] = FALSE;
			}
		}

		// See if the key has been pressed long enough to go into holding state.

		if( ctrlkey_Presstime[ keyNo ] ) {
			if( (schedule_time - ctrlkey_Presstime[ keyNo ]) > ctrlkey_HOLDING_THRESHOLD )
			ctrlkey_Holding[ keyNo ] = TRUE;

			events_Push( e_KEY_HOLDING, 0,
				hw_DeviceID, ctrlkey_Key2Function[keyNo], e_KEY_HOLDING,
				keyNo, 0 );
		}
	}

	_CNIE = 1;
}


//---------------------------------------------------------------------------------------------
// Input Change ISR

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
	static unsigned short lastTimer;
	static unsigned short lastState;

	unsigned short elapsed;
	unsigned short currentState;
	unsigned char diffKeys;
	unsigned char keyNo;

	_CNIE = 0;
	_CNIF = 0;

	elapsed = (TMR1>=lastTimer) ?
			TMR1 - lastTimer :
			0xFFFF - (lastTimer - TMR1);

	// Ignore Bounces.
	if( elapsed < 2 ) goto done;

	currentState = ctrlkey_ReadKeys();

	// Ignore events that don't change anything.
	diffKeys = lastState ^ currentState;
	if( ! diffKeys ) goto done;

	for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {

		// Did this key change?
		if( diffKeys & 0x01 ) {

			// Is this a new key press?
			if( (currentState & 0x0001) == 0 ) {
				ctrlkey_Presstime[ keyNo ] = schedule_time;
				ctrlkey_Releasetime[ keyNo ] = 0;
			}

			// Is it a release?
			else if( (currentState&0x0001) == 1 ) {
				if( ctrlkey_Holding[ keyNo ] ) {

					events_Push( e_KEY_RELEASED, 0,
							hw_DeviceID,
							ctrlkey_Key2Function[keyNo],
							e_KEY_RELEASED, keyNo,
							(short)schedule_time );

					ctrlkey_Presstime[ keyNo ] = 0;
					ctrlkey_Releasetime[ keyNo ] = 0;
					ctrlkey_Holding[ keyNo ] = FALSE;
					ctrlkey_ClickCount[ keyNo ] = 0;
				}

				else {

					// The key was released before being considered held, so it is a click.
					// Don't send it until we have determined if it is a double/triple click.

					ctrlkey_Releasetime[ keyNo ] = schedule_time;
					ctrlkey_ClickCount[ keyNo ]++;
				}
			}
		}

		diffKeys = diffKeys >> 1;
		currentState = currentState >> 1;
	}

	lastState = currentState;
	schedule_ResetTaskTimer( ctrlkey_task );

done:
	lastTimer = TMR1;
	_CNIE = 1;
	return;
}
