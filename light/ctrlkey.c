/*
 * ctrlkey.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper We
 */

#include <p24hxxxx.h>
#include <libpic30.h>
#include <stdlib.h>

#include "hw.h"
#include "events.h"
#include "ctrlkey.h"

//---------------------------------------------------------------------------------------------
// Globals

unsigned char ctrlkey_KeyState[ctrlkey_NO_KEYS];
unsigned short ctrlkey_States[3];
unsigned short ctrlkey_Samples;
unsigned char ctrlkey_EventPending;
unsigned char ctrlkey_KeyHolding;


//---------------------------------------------------------------------------------------------

void ctrlkey_Initialize( void ) {
	char keyNo;

	if( ctrlkey_NO_KEYS ) {
	
		for( keyNo=0; keyNo<ctrlkey_NO_KEYS; keyNo++ ) {
			switch( keyNo ) {
				case 0: ctrlkey_KEY_0_DIR = 1; ctrlkey_KEY_0_CN = 1; break;
			}
		}
		
		// Setup Change Notification Interrupts.
	
		CNEN2 = CNEN2bits.CN16IE;	// RB10
		IEC1bits.CNIE = 1;
		_CNIF = 0;
	}

	// Timer 1 will be our debounce and press&hold timer.

	T1CONbits.TSIDL = 1;
	T1CONbits.TCKPS = 0b00;		// Prescaler = 1
	T1CONbits.TCS = 0;			// CLock Select = Fcy.
	PR1 = 0x1B7D;				// Timer cycle.
	_T1IF = 0;
	IEC0bits.T1IE = 1;
	T1CONbits.TON = 1;			// Start Timer.

	ctrlkey_States[2] = ctrlkey_States[1] = ctrlkey_States[0] = ctrlkey_ReadKeys();
}

//---------------------------------------------------------------------------------------------
// Read all defined keys into a bit mask.

unsigned short ctrlkey_ReadKeys( void ) {
	short keyNo;
	unsigned short newstate;

	newstate = 0;

	for( keyNo=ctrlkey_NO_KEYS-1; keyNo>=0; keyNo-- ) {

		newstate = newstate << 1;

		switch( keyNo ) {
			case 0: newstate |= ctrlkey_KEY_0_PIN; break;
		}
	}

	return newstate;
}


//---------------------------------------------------------------------------------------------
// Timer 1 Interrupt. Keep track of all key changes, accept them as de-bounced if they
// are stable over two periods. Then track key pressed times, and generate key clicked,
// key held or key released events accordingly.

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
	static unsigned short holdingSamples[ctrlkey_NO_KEYS];
	unsigned short diffKeys, currentState;
	short keyNo;

	_T1IF = 0;
	ctrlkey_Samples++;

	hw_WDTCounter = ++hw_WDTCounter % 500;
	if( hw_WDTCounter == 0 ) events_Push( event_WDT_RESET, 0, 0 );

	if( ctrlkey_KeyHolding ) {
		for( keyNo=0; keyNo<ctrlkey_NO_KEYS; keyNo++ ) {
			if( holdingSamples[keyNo] ) {
				holdingSamples[keyNo]++;
				if( holdingSamples[keyNo] == ctrlkey_HOLDING_THRESHOLD ) {
					events_Push( event_KEY_HOLDING, keyNo, ctrlkey_Samples );
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

				for( keyNo=0; keyNo<ctrlkey_NO_KEYS; keyNo++ ) {

					// Did this key change?
					if( diffKeys & 0x0001 ) {

						// Is this a new key press?
						if( (holdingSamples[keyNo] == 0) && ((currentState&0x0001) == 0) ) {
							holdingSamples[keyNo]++;
							ctrlkey_KeyHolding = 1;
						}

						// Is it a release?
						else if( (currentState&0x0001) == 1 ) {

							if( holdingSamples[keyNo] < ctrlkey_HOLDING_THRESHOLD )
								events_Push( event_KEY_CLICKED, keyNo, ctrlkey_Samples );
							else
								events_Push( event_KEY_RELEASED, keyNo, ctrlkey_Samples );

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


//---------------------------------------------------------------------------------------------
// Input Change ISR

void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
{
	_CNIF = 0;
	ctrlkey_EventPending++;
}
