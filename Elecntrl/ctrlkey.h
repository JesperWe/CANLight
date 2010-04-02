/*
 * ctrlkey.h
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#ifndef CTRLKEY_H_
#define CTRLKEY_H_

#define ctrlkey_MAX_NO_KEYS 3

#define ctrlkey_HOLDING_THRESHOLD 300
#define ctrlkey_DOUBLECLICK_THRESHOLD 500


//---------------------------------------------------------------------------------------------
// Globals.

extern unsigned short ctrlkey_NoKeys;
extern unsigned char ctrlkey_ClickCount[ ctrlkey_MAX_NO_KEYS ];
extern unsigned char ctrlkey_Holding[ ctrlkey_MAX_NO_KEYS ];
extern unsigned long ctrlkey_Presstime[ ctrlkey_MAX_NO_KEYS ];
extern unsigned long ctrlkey_Releasetime[ ctrlkey_MAX_NO_KEYS ];


//---------------------------------------------------------------------------------------------
// Prototypes.

void ctrlkey_Initialize( void );
unsigned short ctrlkey_ReadKeys( void );
void ctrlkey_task();

#endif /* CTRLKEY_H_ */
