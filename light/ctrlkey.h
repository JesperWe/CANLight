/*
 * ctrlkey.h
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#ifndef CTRLKEY_H_
#define CTRLKEY_H_

#include "hw.h"
#include "events.h"
#include "config.h"
#include "config_groups.h"

#define ctrlkey_MAX_NO_KEYS 3

#define ctrlkey_HOLDING_THRESHOLD 300
#define ctrlkey_DOUBLECLICK_THRESHOLD 500


//---------------------------------------------------------------------------------------------
// Globals.

extern unsigned char ctrlkey_KeyState[ctrlkey_MAX_NO_KEYS];
extern unsigned short ctrlkey_States[3];
extern unsigned short ctrlkey_Samples;
extern unsigned short ctrlkey_NoKeys;
extern unsigned char ctrlkey_EventPending;
extern unsigned short ctrlkey_ClickPending;
extern unsigned char ctrlkey_ClickCount;
extern unsigned char ctrlkey_KeyHolding;
extern unsigned char ctrlkey_Holding;


//---------------------------------------------------------------------------------------------
// Prototypes.

void ctrlkey_Initialize( void );
unsigned short ctrlkey_ReadKeys( void );
void ctrlkey_task( void *pvParameters );

#endif /* CTRLKEY_H_ */
