/*
 * menu.h
 *
 *  Created on: 5 okt 2009
 *      Author: Jesper W
 */

#ifndef MENU_H_
#define MENU_H_

#define menu_MAX_EVENTS		3
#define menu_NO_DISPLAY_UPDATE	17	// Tells state machine that this event handler wants to manage it's own display.

#define _psv(name)	char name[] __attribute__((space(auto_psv)))

enum menu_States_e {
	S_TERMINATE,
	S_HOMESCREEN,
	S_SETTINGS,
	S_LIGHTING,
	S_ENGINE,
	S_TANKLEVELS,
	S_ENGINE_CALIBRATION,
	S_SAVE_CONFIG,
	S_CONFIG_DO_SAVE,
	S_ENGINE_MONITOR,
	S_LIGHTCONFIG,
	S_BACKLIGHT,
	S_SETTINGSDIST,
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
	char noEvents;								// How many sub-menu events we have.
	int (*handler)(void);						// Optional function to call. NULL means no code called.
	menu_EventData_t events[menu_MAX_EVENTS];	// Possible next events.
} menu_State_t;

extern unsigned short menu_CurStateId, menu_NextStateId, menu_ParentStateId, menu_HandlerStateId;
extern unsigned char menu_Keypress;

void menu_Initialize();
void menu_SetState( unsigned char state );
char menu_ProcessKey( unsigned char keypress );
void menu_Task();

// Event Handler Prototypes

int engine_ThrottleMonitor();
int menu_ParameterSetter( const char* paramNames[], unsigned char noParameters, short parameters[] );
int menu_SaveCalibration();

void engine_ThrottleMonitorUpdater();

// When menu_ActiveHandler is non-zero, it should point to an active event handler that
// requires constant updates. Updating is handled by the T3 Interrupt Event Handler
// in led.c.

extern int (*menu_ActiveHandler)(void);

#endif /* MENU_H_ */
