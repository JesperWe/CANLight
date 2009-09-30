/*
 * events.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper W
 */

#include "events.h"

//---------------------------------------------------------------------------------------------
// Globals.

event_t events_Queue[ events_QUEUESIZE ];
event_t *eventPtr;

unsigned char events_QueueHead;
unsigned char events_QueueTail;
unsigned char events_QueueFull;
unsigned char loopbackEnabled = 1;

//---------------------------------------------------------------------------------------------

void events_Initialize( void ) {
	events_QueueTail = 0;
	events_QueueHead = 0;
	events_QueueFull = 0;
}

//---------------------------------------------------------------------------------------------
// Add event to the queue.
// events_QueueTail points to one behind last, events_QueueHead points to first.

void events_Push( 
		unsigned char eventType, 
		unsigned short nmeaPGN, 
		unsigned char ctrlDev, 
		unsigned char ctrlFunc, 
		unsigned char eventData, 
		unsigned short atTimer )
{
	event_t newEvent;

	// We really can't handle a full queue in any meaningful way.
	// This is an embedded system after all...
	// Ignoring pushes to make sure we don't corrupt.

	if( events_QueueFull ) {
		return;
	}

	newEvent.type = eventType;
	newEvent.ctrlDev = ctrlDev;
	newEvent.ctrlFunc = ctrlFunc;
	newEvent.PGN = nmeaPGN;
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


//---------------------------------------------------------------------------------------------
// Filter events based on the system configuration.
// Returns a configuration event of this device
// that is listening to the current event.
// The fromAccept argument allows scanning to start anywhere in the
// configuration, so multiple listeners can be processed one by one.

cfg_Event_t* event_FindNextListener( cfg_Event_t *fromAccept, event_t* event ) {
	cfg_Event_t *accept;

	// The WDT Reset event does not have a corresponding cfg_Event, but
	// we need to return some none zero value here.

	if( event->type == e_WDT_RESET ) return (cfg_Event_t*)1;

	accept = fromAccept;

	if( accept->group == gEnd ) return 0; // Check for empty MyEvents list.

	while( accept != 0 ) {
		if( accept->ctrlDev != event->ctrlDev ) goto next;
		if( accept->ctrlFunc != event->ctrlFunc ) goto next;

		// For NMEA message events, look at the originating event type.

		if( event->type == e_NMEA_MESSAGE ) {
			if( accept->ctrlEvent != event->data ) goto next; 
		}
		else {
			if( accept->ctrlEvent != event->type ) goto next; 
		}

		// All filters matched, this event should be processed.
		return accept;

	next: accept = accept->next;
	};

	return (cfg_Event_t*)0;
}


//---------------------------------------------------------------------------------------------
// The LoopbackMapper takes an event which originates from our current device,
// and maps it to the target function on the current device that should be
// affected by this event. This is used when the device is controlling local
// (non NMEA) functions.

void event_LoopbackMapper( event_t *event, unsigned char setting ) {

	switch( hw_Type ) {

		case hw_SWITCH: {

			if( event->ctrlFunc == hw_KEY1 ) event->ctrlFunc = hw_LED1;
			if( event->ctrlFunc == hw_KEY2 ) event->ctrlFunc = hw_LED2;
			if( event->ctrlFunc == hw_KEY3 ) event->ctrlFunc = hw_LED3;

			break;
		}
	}
}
