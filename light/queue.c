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

queue_t* queue_Create( short maxEntries, short objectSize ) {

	queue_t *newQueue = malloc( sizeof( queue_t ));
	if( ! newQueue ) queue_OUTOFMEMORY;

	newQueue->objects = malloc( maxEntries * objectSize );
	if( ! newQueue->objects ) queue_OUTOFMEMORY;

	newQueue->capacity = maxEntries;
	newQueue->objectSize = objectSize;
	newQueue->first = 0;
	newQueue->last = 0;
	newQueue->highwater = 0;
	newQueue->status = 0;

	return newQueue;
}

char queue_Send( queue_t* queue, void* object ) {
	char curSize;
	void** objectPointers;

    // We really can't handle a full queue in any meaningful way.
    // This is an embedded system after all...
    // Ignoring pushes to make sure we don't corrupt.

	if( queue->status == 0xFF ) return FALSE;

	objectPointers = (void**)queue->objects;

    memcpy( objectPointers[queue->last], object, queue->objectSize );
    queue->last++;
    queue->last = queue->last % queue->capacity;

    // Calculate high water mark.
    // This is used for optimizing allocated memory from running system data.

    curSize = queue->last > queue->first
    		? queue->last - queue->first
    		: queue->last - queue->first + queue->capacity;

	if( curSize > queue->highwater ) queue->highwater = curSize;

    if( queue->last == queue->first ) {
    	queue->status = 0xFF;
        queue->highwater = queue->capacity;
    }
	return TRUE;
}

char queue_Receive( queue_t* queue, void* object ) {
	void** objectPointers;

	// Empty?
	if( queue->first == queue->last && queue->status != 0xFF ) return FALSE;

	objectPointers = (void**)queue->objects;
	memcpy( object, objectPointers[ queue->first ], queue->objectSize );
    queue->first++;
    queue->first = queue->first % queue->capacity;

	queue->status = 0; // If we were full, we are not anymore.

	return TRUE;
}
