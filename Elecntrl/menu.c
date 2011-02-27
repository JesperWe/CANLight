/*
 * menu.c
 *
 *  Created on: 5 okt 2009
 *      Author: Jesper W
 */

#include <stdio.h>

#include "hw.h"
#include "schedule.h"
#include "display.h"
#include "menu.h"
#include "engine.h"
#include "config.h"
#include "events.h"
#include "led.h"
#include "nmea.h"

_psv(M_TITLE)		= "JM60 Control System";
_psv(M_OK)			= "OK";
_psv(M_CANCEL)		= "Cancel";
_psv(M_YES)			= "Yes";
_psv(M_NO)			= "No";
_psv(M_NA)			= "N/A";
_psv(M_SPACE)		= " ";
_psv(M_SEPARATOR)	= " | ";
_psv(M_SLASH)		= "/";
_psv(M_RARROW)		= "\366";
_psv(M_LARROW)		= "\367";
_psv(M_Settings)	= "Settings";
_psv(M_Engine)		= "Engine";
_psv(M_Lighting)	= "Lighting";
_psv(M_Navigation)	= "Navigation";
_psv(M_Config)		= "Config";
_psv(M_Backlight)	= "Backlight";
_psv(M_Calibration)	= "Calibration";
_psv(M_Ask_Save)	= "Save Settings?";
_psv(M_Saved)		= "Settings Saved!";
_psv(M_Monitor)		= "Monitor";
_psv(M_Group)		= "G";
_psv(M_By)			= "By";
_psv(M_Upload)		= "Upload";
_psv(M_Distribute)	= "Distribute";
_psv(M_)			= "";

//	{ ID, Parent, Text, SubmenuCount, Handler }
//			{ Text, SubmenuID } ...

const menu_State_t menu_States[] = {

	{ S_HOMESCREEN, 0, M_TITLE, 3, 0, {
			{ M_Lighting, S_LIGHTING },
			{ M_Engine, S_ENGINE },
			{ M_Settings, S_SETTINGS }
	} },

	{ S_LIGHTING, S_HOMESCREEN, M_Lighting, 2, 0, {
			{ M_Config, S_LIGHTCONFIG },
			{ M_Backlight, S_BACKLIGHT }
	} },

	{ S_LIGHTCONFIG, S_SAVE_CONFIG, M_Calibration, 0, led_CalibrationParams, {
			{ 0,0 }
	} },

	{ S_BACKLIGHT, S_LIGHTING, M_Backlight, 0, 0, {
			{ 0,0 }
	} },

	{ S_ENGINE, S_HOMESCREEN, M_Engine, 2, 0, {
			{ M_Monitor, S_ENGINE_MONITOR },
			{ M_Calibration, S_ENGINE_CALIBRATION }
	} },

	{ S_ENGINE_MONITOR, S_ENGINE, M_Monitor, 0, engine_ThrottleMonitor, {
			{ 0,0 }
	} },

	{ S_ENGINE_CALIBRATION, S_SAVE_CONFIG, M_Calibration, 0, engine_CalibrationParams, {
			{ 0,0 }
	} },

	{ S_SAVE_CONFIG, -1, M_Ask_Save, 2, 0, {
			{ M_YES, S_CONFIG_DO_SAVE },
			{ M_NO, S_HOMESCREEN }
	} },

	{ S_CONFIG_DO_SAVE, S_HOMESCREEN, M_Saved, 1, menu_SaveCalibration, {
			{ M_OK, S_HOMESCREEN }
	} },

	{ S_SETTINGS, S_HOMESCREEN, M_Settings, 1, 0, {
			{ M_Distribute, S_SETTINGSDIST }
	} },

	{ S_SETTINGSDIST, S_SETTINGS, M_Distribute, 1, nmea_DistributeSettings, {
			{ M_OK, S_HOMESCREEN }
	} },

	{ S_TERMINATE, 0, 0, 0, 0, {
			{ 0,0 }
	} }
};



unsigned short menu_CurStateId, menu_NextStateId, menu_ParentStateId, menu_HandlerStateId;
unsigned char menu_NextIndex;
unsigned char menu_SubItemCount;
unsigned char menu_Parameter = 0;
unsigned char menu_Keypress = 0;
unsigned char menu_HandlerInControl;

menu_State_t* menu_CurState;
int (*menu_ActiveHandler)(void);


//---------------------------------------------------------------------------------------------
// General purpose handler for parameter modifications.
// Each subsystem is responsible for maintaining its own menu texts etc.
// paramNames[0] is a header text, the rest are names of parameters that can be set.

int menu_ParameterSetter( const char* paramNames[], unsigned char noParameters, short parameters[] ) {
	static char delta;
	static char step;
	static char newParameter;

	char line1[5];

	// State machine lets key press through if it is not menu related, so we take care of it here.

	switch( menu_Keypress ) {

		case DISPLAY_KEY_UP: {
			delta = 0;
			step = 10;
			break;
		}

		case DISPLAY_KEY_DOWN: {
			delta = 0;
			step = 1;
			break;
		}

		case DISPLAY_KEY_NORTH: {
			delta = step;
			break;
		}

		case DISPLAY_KEY_SOUTH: {
			delta = -step;
			break;
		}

		case DISPLAY_KEY_WEST: {
			delta = 0;
			newParameter = TRUE;
			if( menu_Parameter > 1 ) menu_Parameter--;
			break;
		}

		case DISPLAY_KEY_EAST: {
			delta = 0;
			newParameter = TRUE;
			if( menu_Parameter+1 < noParameters ) menu_Parameter++;
			break;
		}
	}

	// Now update display and values based on key press.

	if( menu_Parameter == 0 ) {
			menu_Parameter = 1;
			step = 10;
			delta = 0;
			newParameter = TRUE;
	}


	if( newParameter ) {
		newParameter = FALSE;

		display_Clear();
		display_Write( paramNames[0] );
		display_SetPosition(1,2);
		display_Write( paramNames[ menu_Parameter ] );
	}

	else {
		if( ((short)(parameters[ menu_Parameter ]) + delta) < 0 ) {
			parameters[ menu_Parameter ] = 0;
		}
		else if( ((short)(parameters[ menu_Parameter ]) + delta) > 9999 ) {
				parameters[ menu_Parameter ] = 9999;
		}
		else {
			parameters[ menu_Parameter ] += delta;
		}
	}

	display_SetPosition(10,3);
	display_NumberFormat( line1, 4, parameters[ menu_Parameter ] );
	display_Write( line1 );

	return menu_NO_DISPLAY_UPDATE;
}

//---------------------------------------------------------------------------------------------

int menu_SaveCalibration() {
	hw_WriteSettingsFlash();
	menu_ActiveHandler = 0;
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
	int result;
	short StateIndex;

	// Find the right index in the states vector.

	StateIndex = 0;
	while( menu_States[StateIndex].id != S_TERMINATE ) {
		if( menu_States[StateIndex].id == state ) break;
		StateIndex++;
	}

	menu_CurState = &(menu_States[ StateIndex ]);
	menu_CurStateId = menu_CurState->id;

	// Execute our handler if there is one.

	result = 0;
	if( menu_CurState->handler != NULL ) {
		menu_HandlerStateId = menu_CurStateId;
		result = menu_CurState->handler();
		if( result < 0 )
			menu_CurStateId = S_TERMINATE;
	}

	menu_SubItemCount = menu_CurState->noEvents;

	// Any text to display for this state?

	if( menu_CurState->descr && (result != menu_NO_DISPLAY_UPDATE) ) {
		display_Clear();
		display_Home();
		display_Write( menu_CurState->descr );

		// Display menu text if we have sub-menu items.
		// menu_NextIndex controls the display of available items.
		// Since there is only room for two item on the display the "Play" key
		// allows the user to step through a larger number of available commands.

		if( menu_CurState->events[menu_NextIndex].id ) {
			display_SetPosition( 1, display_ROWS_HIGH );
			display_Write( menu_CurState->events[ menu_NextIndex ].descr );
		}

		if( menu_CurState->noEvents > menu_NextIndex+1 ) {

			unsigned char textLength =
					strlen( menu_CurState->events[ menu_NextIndex+1 ].descr );

			display_SetPosition( display_COLS_WIDE-textLength+1, display_ROWS_HIGH );
			display_Write( menu_CurState->events[ menu_NextIndex+1 ].descr );
		}
	}
}


//---------------------------------------------------------------------------------------------

char menu_ProcessKey( unsigned char keypress ) {

	// Pressing the STOP button always stops the current active handler,
	// even if it has taken over the display control.

	menu_Keypress = 0;
	if( keypress == DISPLAY_KEY_STOP ) {
		menu_ActiveHandler = 0;
		menu_HandlerInControl = FALSE;
	}

	if( menu_HandlerInControl ) return 0;

	menu_NextStateId = menu_CurStateId;

	switch( keypress ) {

		case DISPLAY_KEY_STOP: {
				menu_Parameter = 0;
				if( menu_CurState->id == S_HOMESCREEN ) return 1;
				if( menu_CurState->parent > 0 )
					menu_NextStateId = menu_CurState->parent;
				menu_NextIndex = 0;
				engine_JoystickCalibrationMonitor = FALSE;
				break;;
			}

		case DISPLAY_KEY_PLAY: { // Show more available commands if there are any.
				if( menu_NextIndex+2 < menu_SubItemCount )
					{ menu_NextIndex += 2;	}
				else
					{ menu_NextIndex = 0; }
				break;
			}

		case DISPLAY_KEY_LEFT: {
				if( ! menu_CurState->events[menu_NextIndex].id ) return 1;
				menu_ParentStateId = menu_CurStateId;
				menu_NextStateId = menu_CurState->events[menu_NextIndex].id;
				menu_NextIndex = 0;
				break;
			}

		case DISPLAY_KEY_RIGHT: {
				if( ! menu_CurState->events[menu_NextIndex+1].id ) return 1;
				menu_ParentStateId = menu_CurStateId;
				menu_NextStateId = menu_CurState->events[menu_NextIndex+1].id;
				menu_NextIndex = 0;
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
				return 1;
			}

		default: return 0; // Means we don't know what to do with this key.
	}

	menu_SetState( menu_NextStateId );
	return 1;
}

//---------------------------------------------------------------------------------------------
// Process key-presses from the I2C Display Task.
// Check if we have an active event handler function from the
// menu state machine. This function should then be run every time
// a key is pressed, or if no keys are pressed twice per second.

void menu_Task() {
	char key;

	// First see if there are any special display tricks to be done.

	if( engine_JoystickCalibrationMonitor ) {
		char line1[5];
		static short lastLevel;

		if( lastLevel != events_LastLevelSetInfo ) {
			display_SetPosition(10,4);
			display_NumberFormat( line1, 4, events_LastLevelSetInfo );
			display_Write( line1 );
			lastLevel = events_LastLevelSetInfo;
		}
	}

	// Any keys pressed?

	if( queue_Receive( display_Queue, &key ) ) {
		if( menu_ProcessKey( key ) ) {
			return;
		}

		// We have a key that was not processed by the menu handler.

		if( menu_ActiveHandler != 0 ) {
			menu_Keypress = key;
			menu_ActiveHandler();
			return;
		}
	}
}

