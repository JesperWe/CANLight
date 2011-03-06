/*
 * queue
 *
 *  Created on: 10 dec 2009
 *      Author: Jesper W
 *
 *  Simplistic general purpose object queue.
 *  Fixed size queue with fixed equal size objects.
 *  Objects are copied into and out from local memory at Send/Receive.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "queue.h"


//---------------------------------------------------------------------------------------------
// Allocate memory for a new queue.
//
// NB!! objectSize must be an even number of bytes.

queue_t* queue_Create( short maxEntries, short objectSize ) {

	queue_t *newQueue = malloc( sizeof( queue_t ));
	if( ! newQueue ) queue_STOP_ON_OUTOFMEMORY;

	newQueue->objects = malloc( maxEntries * objectSize );
	if( ! newQueue->objects ) queue_STOP_ON_OUTOFMEMORY;

	newQueue->capacity = maxEntries;
	newQueue->objectSize = objectSize;
	newQueue->first = 0;
	newQueue->last = 0;
	newQueue->highwater = 0;
	newQueue->status = 0;

	return newQueue;
}


//---------------------------------------------------------------------------------------------

char queue_Send( queue_t* toQueue, void* object ) {
	unsigned char curSize;
	void** objectPtr;

    // We really can't handle a full queue in any meaningful way.
    // This is an embedded system after all...
    // Ignoring pushes to make sure we don't corrupt.

	if( toQueue->status == 0xFF ) return FALSE;

	objectPtr = toQueue->objects + ( toQueue->objectSize * toQueue->last)/2;

    memcpy( objectPtr, object, toQueue->objectSize );
    toQueue->last++;
    toQueue->last = toQueue->last % toQueue->capacity;

    // Calculate high water mark.
    // This is used for optimizing allocated memory from running system data.

    curSize = toQueue->last > toQueue->first
    		? toQueue->last - toQueue->first
    		: toQueue->last - toQueue->first + toQueue->capacity;

	if( curSize > toQueue->highwater ) toQueue->highwater = curSize;

    if( toQueue->last == toQueue->first ) {
    	toQueue->status = 0xFF;
        toQueue->highwater = toQueue->capacity;
    }
	return TRUE;
}


//---------------------------------------------------------------------------------------------

char queue_Receive( queue_t* fromQueue, void* object ) {
	void** objectPtr;

	if( queue_Empty( fromQueue ) ) return FALSE;

	objectPtr = fromQueue->objects + ( fromQueue->objectSize * fromQueue->first)/2;

	memcpy( object, objectPtr, fromQueue->objectSize );
    fromQueue->first++;
    fromQueue->first = fromQueue->first % fromQueue->capacity;

	fromQueue->status = 0; // If we were full, we are not anymore.

	return TRUE;
}


//---------------------------------------------------------------------------------------------

unsigned char queue_Empty( queue_t* queue ) {
	if( queue->first == queue->last && queue->status != 0xFF ) return TRUE;
	return FALSE;
}
