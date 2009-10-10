/*
 * menu.c
 *
 *  Created on: 5 okt 2009
 *      Author: Jesper W
 */

#include "hw.h"
#include "display.h"
#include "menu.h"
#include "engine.h"


_psv(M_TITLE)		= "JM60 Control System";
_psv(M_OK)			= "OK";
_psv(M_CANCEL)		= "Cancel";
_psv(M_YES)			= "Yes";
_psv(M_NO)			= "No";
_psv(M_SPACE)		= " ";
_psv(M_SEPARATOR)	= " | ";
_psv(M_Settings)	= "Settings";
_psv(M_Engine)		= "Engine";
_psv(M_Lighting)	= "Lighting";
_psv(M_Navigation)	= "Navigation";
_psv(M_Config)		= "Config";
_psv(M_Backlight)	= "Backlight";
_psv(M_)			= "";

menu_State_t menu_States[] = {

	{ S_HOMESCREEN, 0, M_TITLE, 0, {
			{ M_Lighting, S_LIGHTING },
			{ M_Engine, S_ENGINE },
			{ M_Settings, S_SETTINGS },
			{ 0, 0 }
	} },

	{ S_SETTINGS, S_HOMESCREEN, M_Settings, 0, {
			{ 0, 0 }
	} },

	{ S_LIGHTING, S_HOMESCREEN, M_Lighting, 0, {
			{ M_Config, S_LIGHTCONFIG },
			{ M_Backlight, S_BACKLIGHT },
			{ 0, 0 }
	} },

	{ S_LIGHTCONFIG, S_LIGHTING, M_Lighting, 0, {
			{ 0,0 }
	} },

	{ S_BACKLIGHT, S_LIGHTING, M_Backlight, 0, {
			{ 0,0 }
	} },

	{ S_ENGINE, S_HOMESCREEN, M_Engine, menu_Engine, {
			{ 0,0 }
	} },

	{ S_TERMINATE, 0, 0, 0, {
			{ 0,0 }
	} }
};



unsigned short menu_CurStateId, menu_NextStateId, menu_ParentStateId;
unsigned char menu_NextIndex;
menu_State_t* menu_CurState;
int (*menu_ActiveHandler)(void);


//---------------------------------------------------------------------------------------------
// Event handlers for menu items that have custom code.

int menu_Engine() {
	display_SetPosition(1,2);
	display_Write("Throttle:");
	display_SetPosition(1,3);
	display_Write("Gearbox:");

	menu_ActiveHandler = menu_Engine_Status;
	return 0;
}


int menu_Engine_Status() {
	unsigned char gear;

	display_HorizontalBar( 10, 2, engine_ThrottleSetting );

	switch(engine_GearboxSetting) {
		case -50: { gear = 1; break; }
		case   0: { gear = 25; break; }
		case  50: { gear = 49; break; }
	}

	display_HorizontalBar( 10, 3, gear );
	return 0;
}


//---------------------------------------------------------------------------------------------
// The 3 functions below handle the menu state machine.

void menu_Initialize() {

	menu_NextStateId = 0;
	menu_ParentStateId = 0;

	menu_SetState( S_HOMESCREEN );
}


void menu_SetState( unsigned char state ) {
	int result, nNext;

	// Find the right index in the states vector.

	menu_CurStateId = 0;
	while( menu_States[menu_CurStateId].id != S_TERMINATE ) {
		if( menu_States[menu_CurStateId].id == state ) break;
		menu_CurStateId++;
	}

	menu_CurState = &(menu_States[ menu_CurStateId ]);

	// Any text to display for this state?

	if( menu_CurState->descr ) {
		display_Clear();
		display_Home();
		display_Write( menu_CurState->descr );
	}

	// Execute our handler if there is one.

	if( menu_CurState->handler != NULL ) {
		result = menu_CurState->handler();
		if( result < 0 )
			menu_CurStateId = S_TERMINATE;
	}

	// Count number of possible next states.

	nNext = 0;
	while( menu_CurState->events[nNext].id ) nNext++;

	// Display menu text if we have submenu items.

	menu_NextIndex = 0;

	if( menu_CurState->events[menu_NextIndex].id ) {
		display_SetPosition( 1, display_ROWS_HIGH );
		display_Write( menu_CurState->events[ menu_NextIndex ].descr );
	}

	if( menu_CurState->events[menu_NextIndex+1].id ) {

		unsigned char textLength =
				strlen( menu_CurState->events[ menu_NextIndex+1 ].descr );

		display_SetPosition( display_COLS_WIDE-textLength+1, display_ROWS_HIGH );
		display_Write( menu_CurState->events[ menu_NextIndex+1 ].descr );
	}
}


void menu_ProcessKey( unsigned char keypress ) {

	switch( keypress ) {

		case DISPLAY_KEY_STOP: {
				menu_ActiveHandler = 0;
				if( menu_CurState->id == S_HOMESCREEN ) return;
				menu_NextStateId = menu_CurState->parent;
				break;;
			}

		case DISPLAY_KEY_PLAY: {
				menu_NextIndex += 2;
				break;
			}

		case DISPLAY_KEY_LEFT: {
				if( ! menu_CurState->events[menu_NextIndex].id ) return;
				menu_ParentStateId = menu_CurStateId;
				menu_NextStateId = menu_CurState->events[menu_NextIndex].id;
				menu_SetState( menu_NextStateId );
				break;
			}

		case DISPLAY_KEY_RIGHT: {
				if( ! menu_CurState->events[menu_NextIndex+1].id ) return;
				menu_ParentStateId = menu_CurStateId;
				menu_NextStateId = menu_CurState->events[menu_NextIndex+1].id;
				break;
			}

		// Not part of the State Machine, just switch backlight on or off.
		case DISPLAY_KEY_ONOFF: {
				if( display_IsOn ) {
					display_Off();
					display_IsOn = 0;
				}
				else {
					display_On();
					display_IsOn = 1;
				}
				return;
			}
	}

	menu_SetState( menu_NextStateId );
	return;
}
