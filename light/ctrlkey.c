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

//---------------------------------------------------------------------------------------------
// Globals

unsigned char ctrlkey_KeyState[ctrlkey_MAX_NO_KEYS];
unsigned short ctrlkey_States[3];
unsigned short ctrlkey_Samples;
unsigned short ctrlkey_NoKeys;
unsigned char ctrlkey_EventPending;
unsigned short ctrlkey_ClickPending;
unsigned char ctrlkey_KeyHolding;
unsigned char ctrlkey_Holding;
unsigned char ctrlkey_ClickCount;
unsigned char ctrlkey_ClickPendingKeyNo;

static const unsigned char ctrlkey_Key2Function[] = { hw_KEY1, hw_KEY2, hw_KEY3 };


//---------------------------------------------------------------------------------------------

void ctrlkey_Initialize( void ) {

	ctrlkey_EventPending = 0;
	ctrlkey_ClickPending = 0;
	ctrlkey_KeyHolding = 0;
	ctrlkey_Holding = 0;
	ctrlkey_ClickCount = 0;

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

	if( ctrlkey_NoKeys != 0 )
		ctrlkey_States[2] = ctrlkey_States[1] = ctrlkey_States[0] = ctrlkey_ReadKeys();
}

//---------------------------------------------------------------------------------------------
// Read all defined keys into a bit mask.

unsigned short ctrlkey_ReadKeys( void ) {
	unsigned short newstate;

	newstate = 0;

	newstate |= hw_ReadPort( hw_KEY1 );
	newstate |= hw_ReadPort( hw_KEY2 ) << 1;
	newstate |= hw_ReadPort( hw_KEY3 ) << 2;

	return newstate;
}


//---------------------------------------------------------------------------------------------
// Control Key Task
// Keep track of all key changes, accept them as debounced if they
// are stable over two periods. Then track key pressed times, and generate key clicked,
// key held or key released events accordingly.
//

void ctrlkey_task( void *pvParameters ) {
	static unsigned short holdingSamples[ctrlkey_MAX_NO_KEYS];
	unsigned short diffKeys, currentState;
	short keyNo;

	while(1) {
		ctrlkey_Samples++;

		// If we have pending clicks waiting for a double/triple sequence check
		// and time has run out, ship it!

		ctrlkey_ClickPending++;

		if( ctrlkey_ClickCount != 0 && ctrlkey_ClickPending > ctrlkey_DOUBLECLICK_THRESHOLD ) {
			unsigned char clickEvent = e_KEY_CLICKED;
			if( ctrlkey_ClickCount == 2 ) clickEvent = e_KEY_DOUBLECLICKED;
			if( ctrlkey_ClickCount > 2 ) clickEvent = e_KEY_TRIPLECLICKED;

			events_Push( clickEvent, 0,
					hw_DeviceID,
					ctrlkey_Key2Function[ctrlkey_ClickPendingKeyNo],
					clickEvent, ctrlkey_ClickPendingKeyNo,
					ctrlkey_Samples );

			ctrlkey_ClickPending = 0;
			ctrlkey_ClickCount = 0;
		}

		// We also use this timer for the heartbeat timers. (Slow = 3s / Fast = 0.3s)
		// In addition, if we are in the middle of a PWM LED fade, heartbeats will
		// step the fade level.

		/*hw_HeartbeatCounter = ++hw_HeartbeatCounter % hw_SLOW_HEARTBEAT_MS;
		if( (hw_HeartbeatCounter % hw_FAST_HEARTBEAT_MS) == 0 && ctrlkey_Holding ) {
			events_Push( e_FAST_HEARTBEAT, 0, hw_DeviceID, 0, e_FAST_HEARTBEAT, 0, hw_HeartbeatCounter );
		}
		if( hw_HeartbeatCounter == 0 ) {
			events_Push( e_SLOW_HEARTBEAT, 0, hw_DeviceID, 0, e_SLOW_HEARTBEAT, 0, hw_HeartbeatCounter );
		}



		if( hw_Actuators_Installed ) engine_InterruptService();*/


		// Now see if we have a key depressed long enough to generate a holding event.

		if( ctrlkey_KeyHolding ) {
			for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {
				if( holdingSamples[keyNo] ) {
					holdingSamples[keyNo]++;
					if( holdingSamples[keyNo] == ctrlkey_HOLDING_THRESHOLD ) {
						events_Push( e_KEY_HOLDING, 0,
							hw_DeviceID, ctrlkey_Key2Function[keyNo], e_KEY_HOLDING,
							keyNo, ctrlkey_Samples );
					}
				}
			}
		}

		if( ctrlkey_EventPending ) {

			currentState = ctrlkey_ReadKeys();
			ctrlkey_States[1] = ctrlkey_States[0];
			ctrlkey_States[0] = currentState;

			// Stable = Same value for two periods.
			if(ctrlkey_States[1] == ctrlkey_States[0]) {

				// Any change from currently reported key states?
				if(ctrlkey_States[1] != ctrlkey_States[2]) {
					diffKeys = ctrlkey_States[1] ^ ctrlkey_States[2];

					ctrlkey_KeyHolding = 0;

					for( keyNo=0; keyNo<ctrlkey_NoKeys; keyNo++ ) {

						// Did this key change?
						if( diffKeys & 0x0001 ) {

							// Is this a new key press?
							if( (holdingSamples[keyNo] == 0) && ((currentState&0x0001) == 0) ) {
								holdingSamples[keyNo]++;
								ctrlkey_KeyHolding = 1;
							}

							// Is it a release?
							else if( (currentState&0x0001) == 1 ) {

								if( holdingSamples[keyNo] > ctrlkey_HOLDING_THRESHOLD ) {

									events_Push( e_KEY_RELEASED, 0,
											hw_DeviceID,
											ctrlkey_Key2Function[keyNo],
											e_KEY_RELEASED, keyNo,
											ctrlkey_Samples );
								}

								else {

									// The key was released before being considered held, so it is a click.
									// Don't send it until we have determined if it is a double/triple click.

									ctrlkey_ClickCount++;
									ctrlkey_ClickPending = 0;
									ctrlkey_ClickPendingKeyNo = keyNo;
								}

								holdingSamples[keyNo] = 0;
							}
						}

						// If the key didn't change, check if it is being held.
						else {
							if( holdingSamples[keyNo] ) ctrlkey_KeyHolding = 1;
						}

						diffKeys = diffKeys >> 1;
						currentState = currentState >> 1;
					}

					// OK new state has been processed. Save it as current.
					ctrlkey_States[2] = ctrlkey_States[1];
				}
				ctrlkey_EventPending = 0;
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
// Input Change ISR

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
{
	_CNIF = 0;
	ctrlkey_EventPending++;
}
