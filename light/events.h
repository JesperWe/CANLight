/*
 * events.h
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include "hw.h"
#include "config_groups.h"
#include "config.h"

#define events_QUEUESIZE	10

enum event_Events {
	/* 00 */ e_KEY_CLICKED,
	/* 01 */ e_KEY_HOLDING,
	/* 02 */ e_KEY_RELEASED,
	/* 03 */ e_KEY_DOUBLECLICKED,
	/* 04 */ e_KEY_TRIPLECLICKED,
	/* 05 */ e_SWITCH_ON,
	/* 06 */ e_SWITCH_OFF,
	/* 07 */ e_SWITCH_FAIL,
	/* 08 */ e_FADE_START,
	/* 09 */ e_FADE_STOP,
	/* 10 */ e_FAST_HEARTBEAT,
	/* 11 */ e_NMEA_MESSAGE,
	/* 12 */ e_NIGHTMODE,
	/* 13 */ e_DAYLIGHTMODE,
	/* 14 */ e_AMBIENT_LIGHT_LEVEL,
	/* 15 */ e_BLACKOUT,
	/* 16 */ e_SLOW_HEARTBEAT,
	/* 17 */ e_NO_EVENTS
};

typedef struct {
	unsigned char type;
	unsigned short PGN;
	unsigned char ctrlDev;
	unsigned char ctrlFunc;
	unsigned char ctrlEvent;
	unsigned char data;
	unsigned short atTimer;
} event_t;


extern event_t events_Queue[];
extern unsigned char events_QueueHead;
extern unsigned char events_QueueTail;
extern unsigned char events_QueueFull;
extern event_t *eventPtr;

void events_Initialize( void );

void events_Push( 
		unsigned char eventType, 
		unsigned short nmeaPGN, 
		unsigned char ctrlDev, 
		unsigned char ctrlFunc,
		unsigned char ctrlEvent,
		unsigned char eventData, 
		unsigned short atTimer );

event_t* events_Pop( void );

// In case of events from one device targeting another function inside that same device
// we are not receiving our own NMEA messages. So we need to generate loopback events in
// our own event queue. This feature can be enabled or disabled.

extern unsigned char loopbackEnabled;

unsigned char event_Discard( event_t* event );
cfg_Event_t* event_FindNextListener( cfg_Event_t *fromAccept, event_t* event );

#endif /* EVENTS_H_ */
