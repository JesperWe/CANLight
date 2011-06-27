/*
 * ctrlkey.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper We
 */

#include "hw.h"
#include "config.h"
#include "events.h"
#include "ctrlkey.h"
#include "engine.h"
#include "schedule.h"
#include "led.h"

//---------------------------------------------------------------------------------------------
// Globals

unsigned short ctrlkey_NoKeys;
unsigned char ctrlkey_Holding[ ctrlkey_MAX_NO_KEYS ];
unsigned long ctrlkey_Presstime[ ctrlkey_MAX_NO_KEYS ];
unsigned long ctrlkey_Releasetime[ ctrlkey_MAX_NO_KEYS ];
unsigned char ctrlkey_ClickCount[ ctrlkey_MAX_NO_KEYS ];
unsigned char ctrlkey_BacklightKey;

static const unsigned char ctrlkey_Key2Port[] = { hw_KEY1, hw_KEY2, hw_KEY3 };


//---------------------------------------------------------------------------------------------

void ctrlkey_Initialize( void ) {

	switch( hw_Type ) {

		case hw_KEYPAD:	 {
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

		default: {
			ctrlkey_NoKeys = 0; return;
		}
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

#define ctrlkey_DOUBLECLICK_THRESHOLD schedule_SECOND/4
#define ctrlkey_HOLDING_THRESHOLD schedule_SECOND/2

void ctrlkey_task() {
	unsigned char keyNo;
	unsigned char event;
	unsigned char port;
	unsigned char groupId;

	_CNIE = 0; // Disable interrupts while we process, otherwise confusion might ensue...

	for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {

		port = ctrlkey_Key2Port[ keyNo ];

		// If holding, check that we still are. This event is somehow missed sometimes?

		if( ctrlkey_Holding[ keyNo ] ) {

			// Don't fall asleep while holding.
			hw_StayAwakeTimer = schedule_SECOND;

			if( hw_ReadPort( port ) == 0 ) continue;
	
			groupId = config_GetGroupIdForPort( port );

			events_Push( e_IO_EVENT, 0,
					groupId, hw_DeviceID,
					port, e_KEY_RELEASED, keyNo,
					(short)schedule_time );

			ctrlkey_Presstime[ keyNo ] = 0;
			ctrlkey_Releasetime[ keyNo ] = 0;
			ctrlkey_Holding[ keyNo ] = FALSE;
			ctrlkey_ClickCount[ keyNo ] = 0;

			led_FadeMaster = 0;
		}

		if( ctrlkey_Releasetime[ keyNo ] ) {
			if( (schedule_time - ctrlkey_Releasetime[ keyNo ]) > ctrlkey_DOUBLECLICK_THRESHOLD ) {
				switch( ctrlkey_ClickCount[ keyNo ] ) {
				case 2: { event = e_KEY_DOUBLECLICKED; break; }
				case 3: { event = e_KEY_TRIPLECLICKED; break; }
				default: { event = e_KEY_CLICKED; break; }
				}

				groupId = config_GetGroupIdForPort( port );

				// For backlight event we send the desired result rather than the event
				// that caused it. This way listening devices cannot fall out of sync.

				if( groupId == config_GROUP_BROADCAST ) {
					if( led_CurrentLevel[ led_RED ] == 0 ) event = e_TURN_ON;
					else event = e_TURN_OFF;
	 				events_Push( e_IO_EVENT, 0,
							config_CurrentGroup, hw_DeviceID,
							port, event, keyNo,
							(short)ctrlkey_Presstime[ keyNo ] );
					groupId = config_CurrentGroup;
				}

				else {
	 				events_Push( e_IO_EVENT, 0,
							groupId, hw_DeviceID,
							port, event, keyNo,
							(short)ctrlkey_Presstime[ keyNo ] );
				}

				ctrlkey_ClickCount[ keyNo ] = 0;
				ctrlkey_Presstime[ keyNo ] = 0;
				ctrlkey_Releasetime[ keyNo ] = 0;
				ctrlkey_Holding[ keyNo ] = FALSE;
			}
			continue;
		}

		// See if the key has been pressed long enough to go into holding state.

		if( ctrlkey_Presstime[ keyNo ] ) {
			if( (schedule_time - ctrlkey_Presstime[ keyNo ]) > ctrlkey_HOLDING_THRESHOLD ) {

				ctrlkey_Holding[ keyNo ] = TRUE;
				groupId = config_GetGroupIdForPort( port );

				led_FadeMaster = 0xFF;

				if( groupId != config_GROUP_BROADCAST ) {
					events_Push( e_IO_EVENT, 0,
						groupId, hw_DeviceID,
						port, e_KEY_HOLDING, keyNo,
						(short)(schedule_time - ctrlkey_Presstime[ keyNo ]) );
				}
				else {
					// Everyone is listening to this key, send levels using current device as master,
					// rather than making one listener master. (Avoids heavy bus traffic caused by a large number
					// of listeners fighting to become fade masters).

					ctrlkey_BacklightKey = keyNo;
					schedule_AddTask( ctrlkey_SendBackligtLevelTask, schedule_SECOND/4 );
				}

			}
		}
	}

	_CNIE = 1;
}

//---------------------------------------------------------------------------------------------
// Special task if we have a key that controls backlight, and it is being held.
// Dim light level up/down and tell the network to do the same.

void ctrlkey_SendBackligtLevelTask() {
	static float backlightStep;
	if( backlightStep == 0 ) backlightStep = -0.05;
	led_StepDimmer( &backlightStep, led_RED, hw_LED_RED, e_SET_BACKLIGHT_LEVEL );
	if( ! ctrlkey_Holding[ ctrlkey_BacklightKey ] ) schedule_Finished();
}

//---------------------------------------------------------------------------------------------
// Input Change Notification ISR. Takes care of debouncing key presses and keeping track
// of key states.

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
	static unsigned long lastTimer = 0;
	static unsigned short lastState;

	unsigned long elapsed;
	unsigned short currentState;
	unsigned char diffKeys;
	unsigned char keyNo;

	_CNIE = 0;
	_CNIF = 0;

	hw_StayAwakeTimer = schedule_SECOND*2;

	currentState = ctrlkey_ReadKeys();

	elapsed = schedule_time - lastTimer;

	// Ignore Bounces.
	if( elapsed < 3 ) { _CNIE = 1; return; }

	// Ignore events that don't change anything.
	if( lastState == currentState ) { _CNIE = 1; return; }

	diffKeys = lastState ^ currentState;
	lastState = currentState;

	for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {

		// Did this key change?
		if( diffKeys & 0x01 ) {

			// Is this a new key press?
			if( (currentState & 0x0001) == 1 ) {
				ctrlkey_Presstime[ keyNo ] = schedule_time;
				ctrlkey_Releasetime[ keyNo ] = 0;
			}

			// Is it a release?
			else {
				if( ctrlkey_Holding[ keyNo ] ) {

					unsigned char port = ctrlkey_Key2Port[ keyNo ];

					events_Push( e_IO_EVENT, 0,
							config_GetGroupIdForPort( port ), hw_DeviceID,
							port, e_KEY_RELEASED, keyNo,
							(short)schedule_time );

					ctrlkey_Presstime[ keyNo ] = 0;
					ctrlkey_Releasetime[ keyNo ] = 0;
					ctrlkey_Holding[ keyNo ] = FALSE;
					ctrlkey_ClickCount[ keyNo ] = 0;

					led_FadeMaster = 0;
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

	schedule_ResetTaskTimer( ctrlkey_task );

	lastTimer = schedule_time;
	_CNIE = 1;
	return;
}
