/*
 * schedule.h
 *
 *  Created on: 9 dec 2009
 *      Author: Jesper W
 *
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#define schedule_MAX_NO_TASKS	10

// Number of scheduler ticks per second.
// NB! You can't change this freely without checking if 16 bit timer
// overflows will occur. Prescaling might need to be changed.
#define schedule_SECOND		400

typedef struct schedule_Task_s {
	void (*function)(void);
	short intervalTicks;
	short sleepForTicks;
	short waitedTicks;
} schedule_Task_t;

void schedule_Initialize();
short schedule_AddTask( void (*taskFunction)(void), short tickInterval );
void schedule_ResetTaskTimer( void (*taskFunction)(void) );
void schedule_Run();
void schedule_Sleep( short forTicks );
void schedule_Finished();

extern unsigned char schedule_Running;
extern unsigned char schedule_HaveSleepingTask;
extern unsigned long schedule_time;
extern short schedule_Parameter; // Used if we want to tell some one-time task something

#endif /* SCHEDULE_H_ */
