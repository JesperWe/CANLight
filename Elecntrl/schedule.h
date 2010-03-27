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

typedef struct schedule_Task_s {
	void (*function)(void);
	short intervalTicks;
	short sleepForTicks;
	short waitedTicks;
	char suspended;
} schedule_Task_t;

void schedule_Initialize();
short schedule_AddTask( void (*taskFunction)(void), short tickInterval );
void schedule_Run();
void schedule_Sleep( short forTicks );
void schedule_Suspend();
void schedule_Finished();

extern unsigned char schedule_Running;
extern unsigned long schedule_time;

#endif /* SCHEDULE_H_ */
