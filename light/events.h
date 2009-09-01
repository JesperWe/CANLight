/*
 * events.h
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#define events_QUEUESIZE	10

#define event_KEY_CLICKED	0x01
#define event_KEY_HOLDING	0x02
#define event_KEY_RELEASED	0x03

#define event_WDT_RESET		0x04

#define event_NMEA_MESSAGE	0x05

typedef struct {
	unsigned char type;
	unsigned char data;
	unsigned short atTimer;
} event_t;


extern event_t events_Queue[];
extern unsigned char events_QueueHead;
extern unsigned char events_QueueTail;
extern unsigned char events_QueueFull;
extern event_t *eventPtr;

void events_Initialize( void );
void events_Push( unsigned char eventType, unsigned char eventData, unsigned short atTimer );
event_t* events_Pop( void );

#endif /* EVENTS_H_ */
