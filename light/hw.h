/*
 * hw.h
 *
 *  Created on: 2009-jun-07
 *      Author: sysadm
 */

#ifndef HW_H_
#define HW_H_

#define NOP __builtin_nop()

#define hw_CONFIG_MAGIC_WORD	4711

#define nmea_DRIVER_ENABLE 		PORTBbits.RB11

extern unsigned short __attribute__((space(prog),aligned(_FLASH_PAGE*2))) hw_ConfigData[];
extern int hw_Config[_FLASH_ROW];
extern _prog_addressT hw_ConfigPtr;
extern unsigned short hw_WDTCounter;

void hw_Initialize( void );

#endif /* HW_H_ */
