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
_psv(M_ThrottleMax)	= "Throttle Full";
_psv(M_ThrottleMin)	= "Throttle Idle";
_psv(M_GearNeutral)	= "Gear Neutral";
_psv(M_GearForward)	= "Gear Forward";
_psv(M_GearReverse)	= "Gear Reverse";
_psv(M_JoystickMin)	= "Joystick Min";
_psv(M_JoystickMid)	= "Joystick Center";
_psv(M_JoystickMax)	= "Joystick Max";
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
_psv(M_)			= "";

/*
hw_CAN_RATE,
hw_CAN_EN,
hw_LED_RED,
hw_LED_WHITE,
hw_LED1,
hw_LED2,
hw_LED3,
hw_SWITCH1,
hw_SWITCH2,
hw_SWITCH3,
hw_SWITCH4,
hw_KEY1,
hw_KEY2,
hw_KEY3,
hw_LED_LIGHT,	// Composite port RED+WHITE
hw_BACKLIGHT,	// Virtual Port. */


// These are mnemonic names for the ports as defined in hw.h and the events from evenths.h.
// They are used in the config editing display.
// A mnemonic of "--" means this port/event is not configurable.

_psv(M_PortNames)	= "----*R*W*1*2*3\3301\3302\3303\3304o1o2o3*A*\304";
_psv(M_EventNames)	= ">\373\310\274\021\305\306\321\366\367-------M%--";

int config_FileEditor();

menu_State_t menu_States[] = {

	{ S_HOMESCREEN, 0, M_TITLE, 3, 0, {
			{ M_Lighting, S_LIGHTING },
			{ M_Engine, S_ENGINE },
			{ M_Settings, S_SETTINGS }
	} },

	{ S_SETTINGS, S_HOMESCREEN, M_Settings, 0, 0, {
			{ 0,0 }
	} },

	{ S_LIGHTING, S_HOMESCREEN, M_Lighting, 2, 0, {
			{ M_Config, S_LIGHTCONFIG },
			{ M_Backlight, S_BACKLIGHT }
	} },

	{ S_LIGHTCONFIG, S_LIGHTING, 0, 0, config_FileEditor, {
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

	{ S_ENGINE_CALIBRATION, S_ENGINE_SAVE_CALIBRATION, M_Calibration, 0, menu_EngineCalibrate, {
			{ 0,0 }
	} },

	{ S_ENGINE_SAVE_CALIBRATION, S_ENGINE, M_Ask_Save, 2, 0, {
			{ M_YES, S_ENGINE_DO_SAVE },
			{ M_NO, S_ENGINE }
	} },

	{ S_ENGINE_DO_SAVE, S_ENGINE, M_Saved, 1, menu_EngineSaveCalibration, {
			{ M_OK, S_ENGINE }
	} },

	{ S_TERMINATE, 0, 0, 0, 0, {
			{ 0,0 }
	} }
};



unsigned short menu_CurStateId, menu_NextStateId, menu_ParentStateId, menu_HandlerStateId;
unsigned char menu_NextIndex, menu_NoNext;
unsigned char menu_Parameter = 0;
unsigned char menu_Keypress = 0;
unsigned char menu_HandlerInControl;

menu_State_t* menu_CurState;
int (*menu_ActiveHandler)(void);


//---------------------------------------------------------------------------------------------
// Event handlers for menu items that have custom code.

int menu_EngineCalibrate() {
	char* prompt;
	static short delta, step;
	char line1[5];

	menu_ActiveHandler = menu_EngineCalibrate;

	delta = 0;

	// State machine lets key press through if it is not menu related, so we take care of it here.

	switch( menu_Keypress ) {

		case DISPLAY_KEY_UP: {
			step = 10;
			break;
		}

		case DISPLAY_KEY_DOWN: {
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
			if( menu_Parameter > 1 ) menu_Parameter--;
			break;
		}

		case DISPLAY_KEY_EAST: {
			if( menu_Parameter+1 < p_NO_CALIBRATION_PARAMS ) menu_Parameter++;
			break;
		}
	}

	// Now update display and actuator position based on keypress.

	switch( menu_Parameter ) {

		case 0: {
			menu_Parameter = 1;
			prompt = M_ThrottleMin;
			step = 10;
			break;
		}

		case 1: { prompt = M_ThrottleMin; break; }
		case 2: { prompt = M_ThrottleMax; break; }
		case 3: { prompt = M_GearNeutral; break; }
		case 4: { prompt = M_GearForward; break; }
		case 5: { prompt = M_GearReverse; break; }
		case 6: { prompt = M_JoystickMin; break; }
		case 7: { prompt = M_JoystickMid; break; }
		case 8: { prompt = M_JoystickMax; break; }
		default: { prompt = M_NA; break; }
	}

	// No delta means new parameter. Update display!

	if( delta == 0 ) {
		display_Clear();
		display_Write( M_Engine );
		display_Write( M_SPACE );
		display_Write( M_Calibration );
		display_SetPosition(1,2);
		display_Write( prompt );
	}

	else {
		engine_Calibration[menu_Parameter] += delta;
		if( engine_Calibration[menu_Parameter] < 0 ) engine_Calibration[menu_Parameter] = 0;
	}

	switch( menu_Parameter ) {
		case 1: { engine_SetThrottle(0); break; }
		case 2: { engine_SetThrottle(100); break; }
		case 3: { engine_SetGear(0); break; }
		case 4: { engine_SetGear(+1); break; }
		case 5: { engine_SetGear(-1); break; }
	}

	display_SetPosition(10,3);
	display_NumberFormat( line1, 4, engine_Calibration[menu_Parameter] );
	display_Write( line1 );

	return menu_NO_DISPLAY_UPDATE;
}

int menu_EngineSaveCalibration() {
	int i;
	for( i=0; i<p_NO_CALIBRATION_PARAMS; i++ ) {
		hw_Config.engine_Calibration[i] = engine_Calibration[i];
	}
	hw_WriteConfigFlash();

	menu_ActiveHandler = NULL;

	return 0;
}

int config_FileEditor() {
	static unsigned char groupID, maxGroupID, outputID, noTargets;
	unsigned char updateScreen, funcIndex, curTarget;
	char strBuf[10];
	config_File_t groupPtr, targetsPtr, controllersPtr;

	menu_ActiveHandler = config_FileEditor;

	if( menu_Keypress == 0 ) {
		groupID = 0;

		// Scan through the whole file to find the highest group id.

		maxGroupID = 0;
		groupPtr = config_FileFindGroup( 0 );
		while( _findNextGroup( &groupPtr ) ) { maxGroupID++; }
	
		groupPtr = config_FileFindGroup( 0 );
		noTargets = 0;
		outputID = 0;
		updateScreen = TRUE;
	}

	else {
		switch( menu_Keypress ) {

			case DISPLAY_KEY_UP: {
				break;
			}

			case DISPLAY_KEY_DOWN: {
				break;
			}

			case DISPLAY_KEY_NORTH: {
				if( groupID < maxGroupID ) groupID++;
				groupPtr = config_FileFindGroup( groupID );
				updateScreen = TRUE;
				break;
			}

			case DISPLAY_KEY_SOUTH: {
				if( groupID > 0 ) groupID--;
				groupPtr = config_FileFindGroup( groupID );
				updateScreen = TRUE;
				break;
			}

			case DISPLAY_KEY_WEST: {
				if( outputID > 0 ) outputID--;
				break;
			}

			case DISPLAY_KEY_EAST: {
				if( outputID < noTargets ) outputID++;
				break;
			}
		}
	}

	if( updateScreen ) {

		targetsPtr = groupPtr + 1;

		display_Clear();
		display_Write( M_Group );
		display_NumberFormat( strBuf, 3, groupID );
		display_Write( strBuf );
		display_Write( M_SPACE );

		noTargets = 0;

		do {
			funcIndex = 2 * (*(targetsPtr+1));
			noTargets++;
			curTarget = 0;

			display_NumberFormat( strBuf, 0, *targetsPtr );
			display_Write( strBuf );
			strBuf[0] = M_PortNames[funcIndex];
			strBuf[1] = M_PortNames[funcIndex+1];
			strBuf[2] = 0;
			display_Write( strBuf );
			display_Write( M_SPACE );
		} while( _findNextTarget(&targetsPtr) );

		// Now display this groups controllers.

		display_SetPosition( 1, 3 );
		display_Write( M_By );
		display_Write( M_SPACE );

		controllersPtr = groupPtr;
		_findControllers( &controllersPtr );

		while( *controllersPtr != config_GroupEnd ) {

			display_NumberFormat( strBuf, 0, *controllersPtr );
			display_Write( strBuf );
			controllersPtr++;

			funcIndex = 2 * (*controllersPtr);
			strBuf[0] = M_PortNames[funcIndex];
			strBuf[1] = M_PortNames[funcIndex+1];
			strBuf[2] = 0;
			display_Write( strBuf );
			display_Write( M_SPACE );

			controllersPtr++;
			strBuf[1] = 0;

			while( *controllersPtr != config_GroupEnd ) {
				strBuf[0] = M_EventNames[*controllersPtr];
				display_Write( strBuf );
				controllersPtr++;
			}

			display_Write( M_SPACE );
			controllersPtr++;
		}
	}

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
	short i;

	// Find the right index in the states vector.

	i = 0;
	while( menu_States[i].id != S_TERMINATE ) {
		if( menu_States[i].id == state ) break;
		i++;
	}

	menu_CurState = &(menu_States[ i ]);
	menu_CurStateId = menu_CurState->id;

	// Execute our handler if there is one.

	if( menu_CurState->handler != NULL ) {
		menu_HandlerStateId = menu_CurStateId;
		result = menu_CurState->handler();
		if( result < 0 )
			menu_CurStateId = S_TERMINATE;
	}

	menu_NoNext = menu_CurState->noEvents;

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

		if( menu_CurState->events[menu_NextIndex+1].id ) {

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
				menu_ActiveHandler = 0;
				menu_Parameter = 0;
				if( menu_CurState->id == S_HOMESCREEN ) return 1;
				menu_NextStateId = menu_CurState->parent;
				menu_NextIndex = 0;
				break;;
			}

		case DISPLAY_KEY_PLAY: { // Show more available commands if there are any.
				if( menu_NextIndex+2 < menu_NoNext )
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

