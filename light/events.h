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
	e_KEY_CLICKED,
	e_KEY_HOLDING,
	e_KEY_RELEASED,
	e_KEY_DOUBLECLICKED,
	e_KEY_TRIPLECLICKED,
	e_SWITCH_ON,
	e_SWITCH_OFF,
	e_SWITCH_FAIL,
	e_FADE_START,
	e_FADE_STOP,
	e_WDT_RESET,
	e_NMEA_MESSAGE,
	e_NIGHTMODE,
	e_DAYLIGHTMODE,
	e_BACKLIGHT_LEVEL,
	e_BLACKOUT,
	e_NO_EVENTS
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
