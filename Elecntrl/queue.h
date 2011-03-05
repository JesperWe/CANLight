/*
 * queue.h
 *
 *  Created on: 10 dec 2009
 *      Author: sysadm
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#define queue_OUTOFMEMORY while(1) {} // Looping forever is easily detectable in the debugger....

typedef struct queue_s {
	unsigned char capacity;
	unsigned char first;
	unsigned char last;
	unsigned char highwater;
	unsigned char objectSize;
	unsigned char status;
	void** objects;
} queue_t;

queue_t* queue_Create( short maxEntries, short objectSize );

char queue_Send( queue_t* toQueue, void* object );

char queue_Receive( queue_t* fromQueue, void* object );

unsigned char queue_Empty( queue_t* queue );

#endif /* QUEUE_H_ */
