/*
 * events.h
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper W
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#define events_QUEUESIZE	10

enum event_Types_e {
	/* 00 */ e_IO_EVENT,
	/* 01 */ e_NMEA_MESSAGE
};

enum event_Events_e {
	/* 00 */ e_UNKNOWN,
	/* 01 */ e_KEY_CLICKED,
	/* 02 */ e_KEY_HOLDING,
	/* 03 */ e_KEY_RELEASED,
	/* 04 */ e_KEY_DOUBLECLICKED,
	/* 05 */ e_KEY_TRIPLECLICKED,
	/* 06 */ e_SWITCH_ON,
	/* 07 */ e_SWITCH_OFF,
	/* 08 */ e_SWITCH_FAIL,
	/* 09 */ e_FADE_START,
	/* 10 */ e_FADE_MASTER,
	/* 11 */ e_FAST_HEARTBEAT,
	/* 12 */ e_unused,
	/* 13 */ e_NIGHTMODE,
	/* 14 */ e_DAYLIGHTMODE,
	/* 15 */ e_AMBIENT_LIGHT_LEVEL,
	/* 16 */ e_BLACKOUT,
	/* 17 */ e_SLOW_HEARTBEAT,
	/* 18 */ e_THROTTLE_MASTER,
	/* 19 */ e_SET_LEVEL,
	/* 20 */ e_CONFIG_FILE_UPDATE,
	/* 21 */ e_SET_BACKLIGHT_LEVEL,
	/* 22 */ e_NO_EVENTS
};

typedef struct {
	unsigned char type;
	unsigned short PGN;
	unsigned char groupId;
	unsigned char ctrlDev;
	unsigned char ctrlFunc;
	unsigned char ctrlEvent;
	unsigned char data;
	unsigned short info;
} event_t;


extern queue_t* events_Queue;
extern unsigned char events_QueueHead;
extern unsigned char events_QueueTail;
extern unsigned char events_QueueFull;
extern event_t *eventPtr;

void events_Initialize( void );

void events_Push( 
		unsigned char eventType, 
		unsigned short nmeaPGN, 
		unsigned char groupId,
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
extern short events_LastLevelSetInfo;
extern unsigned char events_LastLevelSetData;

unsigned char event_Discard( event_t* event );
config_Event_t* event_FindNextListener( config_Event_t *fromAccept, event_t* event );
void event_Task();

#endif /* EVENTS_H_ */
