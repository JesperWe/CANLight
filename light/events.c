/*
 * events.c
 *
 *  Created on: 2009-jun-06
 *      Author: sysadm
 */

#include "events.h"

//---------------------------------------------------------------------------------------------
// Globals.

event_t events_Queue[ events_QUEUESIZE ];
event_t *eventPtr;

unsigned char events_QueueHead;
unsigned char events_QueueTail;
unsigned char events_QueueFull;


//---------------------------------------------------------------------------------------------

void events_Initialize( void ) {
	events_QueueTail = 0;
	events_QueueHead = 0;
	events_QueueFull = 0;
}

//---------------------------------------------------------------------------------------------
// Add event to the queue.
// events_QueueTail points to one behind last, events_QueueHead points to first.

void events_Push( unsigned char eventType, unsigned char eventData, unsigned short atTimer ) {
	event_t newEvent;

	// We really can't handle a full queue in any meaningful way.
	// Ignoring pushes to make sure we don't corrupt.

	if( events_QueueFull ) {
		return;
	}

	newEvent.type = eventType;
	newEvent.data = eventData;
	newEvent.atTimer = atTimer;

	events_Queue[events_QueueTail] = newEvent;
	events_QueueTail++;
	events_QueueTail = events_QueueTail % events_QUEUESIZE;

	if( events_QueueTail == events_QueueHead ) {
		events_QueueFull = 1;
	}
}

event_t* events_Pop( void ) {

	event_t *eventPtr = 0;

	// Anything to do?
	if( ! events_QueueFull && (events_QueueHead==events_QueueTail)) return eventPtr;

	eventPtr = &(events_Queue[events_QueueHead]);
	events_QueueHead++;
	events_QueueHead = events_QueueHead % events_QUEUESIZE;

	return eventPtr;
}
