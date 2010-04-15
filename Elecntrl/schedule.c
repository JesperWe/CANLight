/*
 * schedule.c
 *
 *  Created on: 9 December 2009
 *      Author: Jesper W
 *
 *  Implements a simplistic and compact scheduler.
 *  No preempting or priority, just plain round robin with possibility to sleep or delay.
 *  Delays are implemented with recursive scheduler calls.
 *
 */

#include "hw.h"
#include "schedule.h"
#include "config.h"
#include "events.h"
#include "nmea.h"
#include "led.h"

short schedule_NoTasks;
schedule_Task_t schedule_Tasks[schedule_MAX_NO_TASKS];
short schedule_Active;
unsigned char schedule_Running = FALSE;
unsigned long schedule_time = 0;
short schedule_Parameter;

void schedule_Initialize() {
	schedule_NoTasks = 0;

    // Timer 1 will be our main task timer.

    T1CONbits.TSIDL = 1;
    T1CONbits.TCKPS = 3;				// Prescaler = 256
    T1CONbits.TCS = 0;					// CLock Select = Fcy.
    PR1 = hw_FCY/256/schedule_SECOND;	// Timer cycle so that the timer will interrupt schedule_SECOND times per second.
    _T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TSIDL = 0;			// Keep running in idle.
    T1CONbits.TON = 1;				// Start Timer.
}

//---------------------------------------------------------------------------------------------

short schedule_AddTask( void (*taskFunction)(void), short tickInterval ) {
	if( schedule_NoTasks == schedule_MAX_NO_TASKS ) return FALSE;
	schedule_Tasks[ schedule_NoTasks ].function = taskFunction;
	schedule_Tasks[ schedule_NoTasks ].intervalTicks = tickInterval;
   	schedule_NoTasks++;
	return schedule_NoTasks;
}

//---------------------------------------------------------------------------------------------
// If the number of ticks waited is reset this task will wait at least it's tickInterval before
// being run next time.

void schedule_ResetTaskTimer( void (*taskFunction)(void) ) {
	unsigned short i;
	for( i=0; i<schedule_NoTasks; i++ ) {
		if( schedule_Tasks[i].function == taskFunction ) {
			schedule_Tasks[i].waitedTicks = 0;
			return;
		}
	}
}

//---------------------------------------------------------------------------------------------
// This is the main event loop for the scheduler.

void schedule_Run() {
	schedule_Task_t *curTask;
	unsigned char idle;
	schedule_Running = TRUE;

	while(1) {
		idle = TRUE;
		for( schedule_Active=0; schedule_Active < schedule_NoTasks; schedule_Active++ ) {

			curTask = &schedule_Tasks[ schedule_Active ];

			// Assuming sleep times will always be longer than the tasks interval.

			if( curTask->sleepForTicks > curTask->waitedTicks ) continue;

			if( curTask->intervalTicks > curTask->waitedTicks ) continue;

			// If we are waking up from a sleep we return to execute the sleeping task
			// at the point where it went to sleep.

			if( curTask->sleepForTicks ) { return; }

			// No rest for the wicked...

			idle = FALSE;
			curTask->sleepForTicks = 0;
			curTask->waitedTicks = 0;
			curTask->function();
		}

		if( idle ) {
			_TRISB15 = 0;
			hw_Sleep();
		}
	}
}


//---------------------------------------------------------------------------------------------
//

void schedule_Sleep( short forTicks ) {
	schedule_Task_t *curTask;
	curTask = &schedule_Tasks[ schedule_Active ];

	curTask->waitedTicks = 0;
	curTask->sleepForTicks = forTicks;

	schedule_Run();

	curTask->sleepForTicks = 0;
}


//---------------------------------------------------------------------------------------------
// Called from a task function that wants to remove itself completely because it is finished.

void schedule_Finished() {
	short iTask;

	schedule_NoTasks--;

	// Are we the last task? If so, killing ourself is simple...

	if( schedule_Active == (schedule_NoTasks - 1)) { return; }

	// No, other tasks have been added after ours.
	// We need to compact the tasks vector before dying.

	for( iTask = schedule_Active; iTask<schedule_NoTasks; iTask++ ) {
		schedule_Tasks[ iTask ] = schedule_Tasks[ iTask+1 ];
	}
}

//---------------------------------------------------------------------------------------------
// Timer 1 Interrupt. All registered tasks have now waited 1 tick more...

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
	_T1IE = 0;
	short i;

	for( i=0; i<schedule_NoTasks; i++) {
		schedule_Tasks[i].waitedTicks++;
	}

	schedule_time++;
	if( hw_SleepTimer > 0 ) hw_SleepTimer--;

    _T1IF = 0;
    _T1IE = 1;
}
