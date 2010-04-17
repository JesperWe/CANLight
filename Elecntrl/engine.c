/*
 * engine.c
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "schedule.h"
#include "nmea.h"
#include "display.h"
#include "menu.h"
#include "engine.h"
#include "events.h"

short	engine_ThrottlePW;
short	engine_GearPW;
short	engine_CurThrottlePW;
short	engine_CurGearPW;

unsigned char	engine_CurMasterDevice;
short			engine_Joystick_Level;
short			engine_LastJoystickLevel;
char			engine_Throttle; // Used by both Joystick, Actuators and I2C!
short			engine_Gear;
unsigned long	engine_GearSwitchTime;
short			engine_ThrottleTimeSteps;
unsigned char engine_JoystickCalibrationMonitor;

unsigned char	engine_TargetThrottle;

char	engine_CurrentGear;
char	engine_TargetGear;

const char* engine_ParamNames[] = {
	"Engine Calibration",
	"Throttle Full",
	"Throttle Idle",
	"Gear Neutral",
	"Gear Forward",
	"Gear Reverse",
	"Joystick Min",
	"Joystick Center",
	"Joystick Max",
};

//---------------------------------------------------------------------------------------------
// Set up OC3 and OC4 to do old fashioned Hobby RC pulse control.
// The pulse train is somewhat arbitrary in frequency, only pulse length counts.
// 0-100% is represented by pulse length 1ms to 2ms, and 50% is 1.5ms.
// We use the same timer (TMR2) as the LED PWM functions.

void engine_Initialize() {

	OC3CONbits.OCSIDL = 0; 		// Stop in idle mode.
	OC3CONbits.OCM = 6; 		// PWM mode.

	OC4CONbits.OCSIDL = 0; 		// Stop in idle mode.
	OC4CONbits.OCM = 6; 		// PWM mode.

	engine_SetGear(0);			// Gear to neutral.
	engine_SetThrottle(0);		// Throttle to idle.

	hw_WritePort( hw_SWITCH3, 0 );
	hw_OutputPort( hw_SWITCH3 ); // Power to Roboteq.
}

//---------------------------------------------------------------------------------------------

void engine_ThrottleInitialize() {
	engine_LastJoystickLevel = engine_UNKNOWN_JOYSTICK;
	engine_CurrentGear = engine_UNKNOWN_GEAR;
}

//---------------------------------------------------------------------------------------------
// Read throttle settings and return True if there was any activity.

#define engine_IDLE_DEADBAND 10

unsigned char engine_ReadThrottleLevel() {
	short diff;
	short calibratedLevel;

	engine_Joystick_Level = ADC_Read( engine_JOYSTICK_AD_CHANNEL );

	// First time since device reset?

	if( engine_LastJoystickLevel == engine_UNKNOWN_JOYSTICK ) {
		engine_LastJoystickLevel = engine_Joystick_Level;
		return 0;
	}

	// Require a significant movement before taking action.

	diff = engine_Joystick_Level - engine_LastJoystickLevel;
	if( diff<3 && diff>-3 ) return 0;
	engine_LastJoystickLevel = engine_Joystick_Level;

	// Scale A/D Converted level down to -100 --  +100 integer, calibrated.

	calibratedLevel = engine_Joystick_Level - hw_Config->engine_Calibration[ p_JoystickMid ];

	if( calibratedLevel < engine_IDLE_DEADBAND && (-calibratedLevel < engine_IDLE_DEADBAND) ) {
		engine_Throttle = 0;
	}
	else if( calibratedLevel > 0  ) {
		engine_Throttle = 100.0 * (float)(calibratedLevel - engine_IDLE_DEADBAND) /
				(float)( hw_Config->engine_Calibration[ p_JoystickMax ] -
						  hw_Config->engine_Calibration[ p_JoystickMid ] );
	}
	else {
		engine_Throttle = 100.0 * (float)(calibratedLevel + engine_IDLE_DEADBAND) /
				(float)( hw_Config->engine_Calibration[ p_JoystickMid ] -
						  hw_Config->engine_Calibration[ p_JoystickMin ] );
	}

	if( engine_Throttle > 100 ) engine_Throttle = 100;
	if( engine_Throttle < -100 ) engine_Throttle = -100;

	return 1;
}


void engine_UpdateActuators() {

	if( engine_ThrottlePW != engine_CurThrottlePW ) {
		OC3RS = engine_ThrottlePW;
		engine_CurThrottlePW = engine_ThrottlePW;
	}

	if (engine_GearPW != engine_CurGearPW ) {
		OC4RS = engine_GearPW;
		engine_CurGearPW = engine_GearPW;
	}
}


//---------------------------------------------------------------------------------------------
// Request a throttle level from 0 to 100.

void engine_RequestThrottle( unsigned char level ) {

	engine_TargetThrottle = level;

	// We can execute the change immediately if no gear change is involved.

	if( engine_CurrentGear == engine_TargetGear ) {
		engine_SetThrottle( level );
		engine_ThrottleTimeSteps = 0;
		return;
	}

	// For 3000rpm = 3sek and 100ms task interval use / 3:
	engine_ThrottleTimeSteps += engine_Throttle / 3;
	engine_SetThrottle( 0 );
}


//---------------------------------------------------------------------------------------------
// Request a gear setting. The actual gear will not be set until engine RPM allows it.

void engine_RequestGear( char direction ) {

	// Do nothing if we are already in the right gear.

	if( direction == engine_CurrentGear ) return;

	// Always go to idle throttle before changing gears.
	// Wait for engine rev to settle. This time is dependent on how high the rev was before
	// gear change was commanded.

	engine_TargetGear = direction;
}


//---------------------------------------------------------------------------------------------
// Actually set throttle to a level from 0 to 100.

void engine_SetThrottle( unsigned char level ) {
	float fLevel;
	float range;

	// Maintain a simulated RPM counter.

	engine_Throttle = level;

	fLevel = ((float)level) / 100.0;

	range = hw_Config->engine_Calibration[ p_ThrottleMax ] - hw_Config->engine_Calibration[ p_ThrottleMin ];
	fLevel = fLevel * range;

	engine_ThrottlePW = hw_Config->engine_Calibration[ p_ThrottleMin ] + (short)fLevel;
	engine_UpdateActuators();
}


//---------------------------------------------------------------------------------------------
// Actually set gear.

void engine_SetGear( char direction ) {

	switch( direction ) {
		case 0: { engine_GearPW = hw_Config->engine_Calibration[ p_GearNeutral ]; break; }
		case 1: { engine_GearPW = hw_Config->engine_Calibration[ p_GearForward ]; break; }
		case -1: { engine_GearPW = hw_Config->engine_Calibration[ p_GearReverse ]; break; }
	}

	// Now do the gear change.

	engine_UpdateActuators();
	engine_CurrentGear = direction;
	engine_GearSwitchTime = schedule_time;
}


//--------------------------------------------------------------------------------------------
// Engine Task:
// Fade approximated engine RPM towards current throttle setting.
// If a gear change has been requested, do it when the target RPM has been  reached.

void engine_ActuatorTask() {

	if( engine_ThrottleTimeSteps > 0 ) {
		engine_ThrottleTimeSteps--;
		return;
	}

	// Any gear change pending?

	if( engine_CurrentGear != engine_TargetGear ) {

		// Require at least 1 seconds between gear changes.

		long interval;
		interval = schedule_time - engine_GearSwitchTime;
		if( interval > schedule_SECOND ) {

			// Go to neutral between forward and reverse.

			if( engine_TargetGear != 0 && engine_CurrentGear != 0 ) {
				engine_SetGear( 0 );
			}

			else {
				engine_SetGear( engine_TargetGear );
				engine_SetThrottle( engine_TargetThrottle );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------
// This task reads the joystick and sends it out as a new throttle level if it has changed.

void engine_JoystickTask() {
	event_t event;

	if( engine_CurMasterDevice != hw_DeviceID ) return;

	// Check that this appliance is configured as a throttle sender.

	if( functionInGroup[ hw_ANALOG ] == 0 ) return;

	if( engine_ReadThrottleLevel() ) {
		event.PGN = 0;
		event.groupId = functionInGroup[ hw_ANALOG ];
		event.ctrlDev = hw_DeviceID;
		event.ctrlFunc = hw_ANALOG;
		event.ctrlEvent = e_SET_LEVEL;
		event.data = engine_Throttle;
		event.info = engine_Joystick_Level;
		nmea_SendEvent( &event );
	}
}


//--------------------------------------------------------------------------------------------
// Menu state machine handlers for engine settings:
// engine_ThrottleMonitor - Set up the display for monitoring throttle levels.
// engine_ThrottleMonitorUpdater - Continuously running task that updates the display.
//

int engine_ThrottleMonitor() {
	display_SetPosition(1,2);
	display_Write("Throttle:");
	display_SetPosition(1,3);
	display_Write("Gearbox:");

	schedule_AddTask( engine_ThrottleMonitorUpdater, schedule_SECOND/2 );
	return menu_NO_DISPLAY_UPDATE;
}

void engine_ThrottleMonitorUpdater() {
	char gear, throttle;
	char numberString[5];

	if( menu_CurStateId != menu_HandlerStateId ) {
		schedule_Finished();
		return;
	}

	// First show exact joystick levels read.

	display_NumberFormat( numberString, 4, events_LastLevelSetInfo );
	display_SetPosition( 17,1 );
	display_Write( numberString );

	// Now show a graphical representation of the resulting actuator positions.

	gear = 0;
	throttle = events_LastLevelSetData;
	if( throttle > 0 ) gear = 1;
	if( throttle < 0 ) {
		gear = -1;
		throttle = -throttle;
	}

	throttle = throttle / 2; // Scale from 0-100 to 0-50

	display_HorizontalBar( 10, 2, throttle );

	switch( gear ) {
		case 0:  { gear = 25; break; }
		case 1:  { gear = 49; break; }
		case -1: { gear =  1; break; }
	}

	display_HorizontalBar( 10, 3, gear );
	return;
}

//--------------------------------------------------------------------------------------------

int engine_CalibrationParams() {

	if( menu_ActiveHandler == 0 ) {
		menu_ActiveHandler = engine_CalibrationParams;
		hw_ReadConfigFlash();
	}

	engine_JoystickCalibrationMonitor = TRUE;

	return menu_ParameterSetter( engine_ParamNames, engine_NO_CALIBRATION_PARAMS, hw_Config->engine_Calibration );
}

//--------------------------------------------------------------------------------------------

int engine_ProcessEvent( event_t *event, unsigned char function ) {
	event_t masterEvent;
	char	throttle;

	if( ! hw_Actuators_Installed ) return 0;

	switch( event->ctrlEvent ) {

	case e_SET_LEVEL: {

		throttle = event->data; // Make a signed value from event->data which is unsigned.

		if( event->ctrlDev != engine_CurMasterDevice ) break;

		if( throttle == 0 ) {
			engine_RequestGear( 0 );
			engine_RequestThrottle( throttle );
		}

		else if( throttle > 0 ) {
			engine_RequestGear( 1 );
			engine_RequestThrottle( throttle );
		}

		else {
			engine_RequestGear( -1 );
			engine_RequestThrottle( -throttle );
		}

		break;
	}

	case e_KEY_CLICKED: {

		// A device becomes the new master.

		if( engine_CurMasterDevice != event->ctrlDev ) {
			engine_CurMasterDevice = event->ctrlDev;
			masterEvent.ctrlEvent = e_THROTTLE_MASTER;
			masterEvent.ctrlFunc = event->ctrlFunc;
			masterEvent.info = engine_CurMasterDevice;
			masterEvent.data = event->data; // Return info about what key was clicked to become master.

			// Turn on power to the Roboteq.

			hw_WritePort( hw_SWITCH3, 1 );

		}

		// The current master want to shut down.

		else {

			engine_CurMasterDevice = 0;

			engine_RequestThrottle( 0 );
			engine_RequestGear( 0 );

			masterEvent.ctrlEvent = e_SWITCH_OFF;
			masterEvent.ctrlFunc = hw_PWM1;
			masterEvent.info = 0;
			masterEvent.data = 0;

			// Wait a while for actuators to settle, then turn off power to the Roboteq.

			schedule_Sleep( schedule_SECOND * 2 );
			hw_WritePort( hw_SWITCH3, 0 );
		}

		nmea_Wakeup();

		masterEvent.ctrlDev = hw_DeviceID;
		masterEvent.groupId = functionInGroup[ function ];

		nmea_SendEvent( &masterEvent );

		break;
	}
	}
	return 0;
}

//--------------------------------------------------------------------------------------------

void engine_SetMaster( event_t *event ) {
	short function;

	engine_CurMasterDevice = event->info;

	if( engine_CurMasterDevice == hw_DeviceID ) {
		hw_AcknowledgeSwitch( event->ctrlFunc, 1 );
	}

	// Our device lost master status. Our device doesn't necessarily use the same button
	// to request master status as the new master, so we need to go through
	// our config to find which indicator to turn off.

	else {
		for( function=0; function<hw_NoFunctions; function++ ) {
			if( functionListenGroup[ function ] == event->groupId ) {
				hw_AcknowledgeSwitch( function, 0 );
			}
		}
	}
}
