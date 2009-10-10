/*
 * menu.h
 *
 *  Created on: 5 okt 2009
 *      Author: Jesper W
 */

#ifndef MENU_H_
#define MENU_H_

#define menu_MAX_EVENTS		6

#define _psv(name)	char name[] __attribute__((space(auto_psv)))

enum menu_States_e {
	S_TERMINATE,
	S_HOMESCREEN,
	S_SETTINGS,
	S_LIGHTING,
	S_ENGINE,
	S_LIGHTCONFIG,
	S_BACKLIGHT,
	menu_NO_STATES
};


//---------------------------------------------------------------------------------------
// One event in the state machine definition structure.

typedef struct menu_EventData_s {
	const char* descr;							// Text to show on the display for the button
	const int id;								// Next state to go to.
} menu_EventData_t;


typedef struct menu_State_s {
	int	id;										// ID of this state.
	int	parent;									// ID of this states parent state.
	char* descr;								// Text to show in top line of front panel display.
	int (*handler)(void);						// Optional function to call. NULL means no code called.
	menu_EventData_t events[menu_MAX_EVENTS];	// Possible next events.
} menu_State_t;

void menu_Initialize();
void menu_SetState( unsigned char state );
void menu_ProcessKey( unsigned char keypress );

// Event Handler Prototypes

int menu_Engine();
int menu_Engine_Status();


// When menu_ActiveHandler is non-zero, it should point to an active event handler that
// requires constant updates. Updating is handled by the T3 Interrupt Event Handler
// in led.c.

extern int (*menu_ActiveHandler)(void);


#endif /* MENU_H_ */
