/*
 * ctrlkey.h
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#ifndef CTRLKEY_H_
#define CTRLKEY_H_


#define ctrlkey_NO_KEYS 0

#define ctrlkey_KEY_0_PIN	PORTBbits.RB10
#define ctrlkey_KEY_0_DIR	TRISBbits.TRISB10
#define ctrlkey_KEY_0_CN	CNEN2bits.CN16IE

#define ctrlkey_HOLDING_THRESHOLD 300

//---------------------------------------------------------------------------------------------
// Globals.

extern unsigned char ctrlkey_KeyState[ctrlkey_NO_KEYS];
extern unsigned short ctrlkey_States[3];
extern unsigned short ctrlkey_Samples;
extern unsigned char ctrlkey_EventPending;
extern unsigned char ctrlkey_KeyHolding;


//---------------------------------------------------------------------------------------------
// Prototypes.

void ctrlkey_Initialize( void );
unsigned short ctrlkey_ReadKeys( void );

#endif /* CTRLKEY_H_ */
