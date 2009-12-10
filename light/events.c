/*
 * events.c
 *
 *  Created on: 2009-jun-06
 *      Author: Jesper W
 */

#include "events.h"
#include "display.h"
#include <stdio.h>

//---------------------------------------------------------------------------------------------
// Globals.

queue_t* events_Queue;
event_t *eventPtr;

unsigned char loopbackEnabled = 1;

//---------------------------------------------------------------------------------------------

void events_Initialize( void ) {
	events_Queue = queue_Create( events_QUEUESIZE, sizeof(event_t) );
}

//---------------------------------------------------------------------------------------------
// Add event to the queue.

void events_Push( 
		unsigned char type,
		unsigned short nmeaPGN, 
		unsigned char ctrlDev, 
		unsigned char ctrlFunc, 
		unsigned char ctrlEvent,
		unsigned char data,
		unsigned short atTimer )
{
	event_t newEvent;

	// Debug Dump...
	//if( type != e_SLOW_HEARTBEAT ){
	//	char line[30];
	//	sprintf( line, "%04d:%02d(%01d) %02x %02d %03d", atTimer, type, events_QueueFull, ctrlFunc, ctrlEvent, data );
	//	display_Write( line );
	//}

	newEvent.type = type;
	newEvent.ctrlDev = ctrlDev;
	newEvent.ctrlFunc = ctrlFunc;
	newEvent.ctrlEvent = ctrlEvent;
	newEvent.PGN = nmeaPGN;
	newEvent.data = data;
	newEvent.atTimer = atTimer;

    queue_Send( events_Queue, &newEvent );
}


//---------------------------------------------------------------------------------------------
// Filter events based on the system configuration.
// Returns a configuration event of this device
// that is listening to the current event.
// The fromAccept argument allows scanning to start anywhere in the
// configuration, so multiple listeners can be processed one by one.

config_Event_t* event_FindNextListener( config_Event_t *fromAccept, event_t* event ) {
	config_Event_t *accept;

	// The WDT Reset event does not have a corresponding cfg_Event, but
	// we need to return some none zero value here.

	if( event->type == e_FAST_HEARTBEAT ) return (config_Event_t*)1;

	accept = fromAccept;

	if( accept->group == gEnd ) return 0; // Check for empty MyEvents list.

	while( accept != 0 ) {
		if( accept->ctrlDev != event->ctrlDev ) goto next;
		if( accept->ctrlFunc != event->ctrlFunc ) goto next;

		// For NMEA message events, look at the originating event type.

		if( event->type == e_NMEA_MESSAGE ) {
			if( accept->ctrlEvent != event->ctrlEvent ) goto next;
		}
		else {
			if( accept->ctrlEvent != event->type ) goto next; 
		}

		// All filters matched, this event should be processed.
		return accept;

	next: accept = accept->next;
	};

	return (config_Event_t*)0;
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
