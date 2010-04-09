/*
 * engine.c
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "schedule.h"
#include "events.h"
#include "nmea.h"
#include "engine.h"
#include "display.h"
#include "menu.h"

short	engine_ThrottlePW;
short	engine_GearPW;
short	engine_CurThrottlePW;
short	engine_CurGearPW;

unsigned char	engine_CurMasterDevice;
short			engine_Joystick_Level;
short			engine_LastJoystickLevel;
unsigned char	engine_Throttle;
short			engine_Gear;
short 			engine_GearSwitchTime;
short 			engine_CurrentRPM;
short 			engine_TargetRPM;

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
// Set up TIMER3 and TIMER4 to do old fashioned Hobby RC pulse control.
// The pulse train is somewhat arbitrary in frequency, only pulse length counts.
// 0-100% is represented by pulse length 1ms to 2ms, and 50% is 1.5ms.

void engine_Initialize() {

	OC3CONbits.OCSIDL = 1; 		// Stop in idle mode.
	OC3CONbits.OCM = 6; 		// PWM mode.

	OC4CONbits.OCSIDL = 1; 		// Stop in idle mode.
	OC4CONbits.OCM = 6; 		// PWM mode.

	TRISCbits.TRISC2 = 0;
	TRISCbits.TRISC0 = 0;

}

//---------------------------------------------------------------------------------------------
// Restore Joystick calibrations from Flash memory.

void engine_ThrottleInitialize() {
	engine_LastJoystickLevel = engine_UNKNOWN_JOYSTICK;
	engine_CurrentGear = engine_UNKNOWN_GEAR;
	engine_SetGear(0);		// Gear to neutral.
	engine_SetThrottle(0);	// Throttle to idle.
}

//---------------------------------------------------------------------------------------------
// Read throttle settings and return True if there was any activity.

unsigned char engine_ReadThrottleLevel() {
	unsigned short range;
	short diff;
	float fLevel;

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

	// Scale A/D Converted level down to 0-99 integer, calibrated to the
	// individual joystick.

	fLevel = (float)engine_Joystick_Level;
	fLevel -= hw_Config->engine_Calibration[ p_JoystickMid ];

	if( fLevel < 0 ) {
		range = hw_Config->engine_Calibration[ p_JoystickMid ] - hw_Config->engine_Calibration[ p_JoystickMin ];
	} else {
		range = hw_Config->engine_Calibration[ p_JoystickMax ] - hw_Config->engine_Calibration[ p_JoystickMid ];
	}

	fLevel = fLevel / (float)range;
	if( fLevel < -1.0 ) fLevel = -1.0;
	if( fLevel > 1.0 ) fLevel = 1.0;

	// Gear forward/reverse.

	engine_Gear = 1;
	if( fLevel < 0.0 ) {
		engine_Gear = 2;
		fLevel = -fLevel;
	}

	if( fLevel < 0.08 ) { // XXX Idling dead-band. Make configurable.
		engine_Gear = 0;
		fLevel = 0;
	}

	// Decrease sensitivity near center.

	fLevel = fLevel * (1.0 + fLevel) / 2.0;

	engine_Throttle = (unsigned char)(fLevel * 100.0);
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
	}
}


//---------------------------------------------------------------------------------------------
// Actually set throttle to a level from 0 to 100.

void engine_SetThrottle( unsigned char level ) {
	float fLevel;
	short range;

	// Maintain a simulated RPM counter that will slowly fade towards the requested value.

	engine_TargetRPM = level * 30;

	fLevel = ((float)level) / 100.0;

	range = hw_Config->engine_Calibration[ p_ThrottleMax ] - hw_Config->engine_Calibration[ p_ThrottleMin ];
	fLevel = fLevel * (float)range;

	engine_ThrottlePW = hw_Config->engine_Calibration[ p_ThrottleMin ] + (short)fLevel;
	engine_UpdateActuators();
}


//---------------------------------------------------------------------------------------------
// Request a gear setting. The actual gear will not be set until engine RPM allows it.

void engine_RequestGear( char direction ) {

	engine_TargetGear = direction;

	// Do nothing if we are already in the right gear.

	if( direction == engine_CurrentGear ) return;

	// Always go to idle throttle before changing gears.
	// Wait for engine rev to settle. This time is dependent on how high the rev was before
	// gear change was commanded.

	engine_SetThrottle( 0 );
}


//---------------------------------------------------------------------------------------------
// Actually set gear.

void engine_SetGear( char direction ) {

	switch( direction ) {
		case 0: { engine_GearPW = hw_Config->engine_Calibration[ p_GearNeutral ]; break; }
		case 1: { engine_GearPW = hw_Config->engine_Calibration[ p_GearForward ]; break; }
		case 2: { engine_GearPW = hw_Config->engine_Calibration[ p_GearReverse ]; break; }
	}

	// Now do the gear change.

	engine_UpdateActuators();
	engine_CurrentGear = direction;
	engine_GearSwitchTime = hw_HeartbeatCounter;
}


//--------------------------------------------------------------------------------------------
// Engine Task:
// Fade approximated engine RPM towards current throttle setting.
// If a gear change has been requested, do it when the target RPM has been  reached.

void engine_ActuatorTask() {

	// Here we assume that the scheduler is running at an approximate tick interval of 1ms
	// and there is by coincidence a good match between typical engine revving down times
	// and a cycle of 1 rpm/ms.

	if( engine_TargetRPM > engine_CurrentRPM ) {
		engine_CurrentRPM += 1000 / schedule_SECOND;
		if( engine_CurrentRPM > engine_TargetRPM ) engine_CurrentRPM = engine_TargetRPM;
	}
	else if( engine_TargetRPM < engine_CurrentRPM ) {
		engine_CurrentRPM -= 1000 / schedule_SECOND;
		if( engine_CurrentRPM < engine_TargetRPM ) engine_CurrentRPM = engine_TargetRPM;
	}

	// Any gear change pending?

	if( engine_CurrentGear != engine_TargetGear ) {
		if( engine_CurrentRPM == engine_TargetRPM ) {

			// Require at least 1 seconds between gear changes. XXX OLD CODE!

			short interval;
			interval = hw_HeartbeatCounter - engine_GearSwitchTime;
			if( interval < 0 ) interval += hw_SLOW_HEARTBEAT_MS;
			if( interval > 1000 ) {

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
}



//--------------------------------------------------------------------------------------------
// This task reads the joystick and sends it out as a new throttle level if it has changed.

void engine_JoystickTask() {
	event_t event;

	if( engine_ReadThrottleLevel() ) {

		if( engine_CurMasterDevice != hw_DeviceID ) {

			// Need to become Master Controller before update!

			event.PGN = 0;
			event.info = 0;
			event.ctrlDev = hw_DeviceID;
			event.ctrlFunc = 0;
			event.ctrlEvent = e_THROTTLE_MASTER;
			event.data = 0;

			nmea_SendEvent( &event );

			engine_CurMasterDevice = hw_DeviceID;
		}

		// Send Throttle and Gear settings on bus.
		// Timing is not interesting here. Use it to communicate the original ADC level from the Joystick.
		// (This can then be used in calibrating the joystick.

		event.PGN = 0;
		event.ctrlDev = hw_DeviceID;
		event.ctrlFunc = engine_Gear;
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
	unsigned char gear, throttle;
	char numberString[5];

	if( menu_CurStateId != menu_HandlerStateId ) {
		schedule_Finished();
		return;
	}

	// First show exact joystick levels read.

	display_NumberFormat( numberString, 4, engine_LastJoystickLevel );
	display_SetPosition( 17,1 );
	display_Write( numberString );

	// Now show a graphical representation of the resulting actuator positions.

	throttle = engine_Throttle>>1; // Scale from 0-100 to 0-50

	display_HorizontalBar( 10, 2, throttle );

	switch( engine_Gear ) {
		case 0: { gear = 25; break; }
		case 1: { gear = 49; break; }
		case 2: { gear =  1; break; }
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

	return menu_ParameterSetter( engine_ParamNames, engine_NO_CALIBRATION_PARAMS, hw_Config->engine_Calibration );
}
