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

short engine_ThrottlePW;
short engine_GearPW;
short engine_CurThrottlePW;
short engine_CurGearPW;

unsigned char engine_CurMasterDevice;
short engine_Joystick_Level;
short engine_LastJoystickLevel;
char engine_Throttle;
char engine_LastThrottle;
short engine_Gear;

unsigned long engine_LastGearTime;
unsigned long engine_LastActuatorUpdate;

short engine_ThrottleTimeSteps;
short engine_GearTimeSteps;
unsigned char engine_JoystickCalibrationMonitor;

unsigned char engine_TargetThrottle;

char engine_CurrentGear;
char engine_TargetGear;

const char* engine_ParamNames[] = {
"Engine Calibration", "Throttle Full", "Throttle Idle", "Gear Neutral", "Gear Forward", "Gear Reverse", "Joystick Min", "Joystick Center", "Joystick Max", "Actuator Timeout"
};

//---------------------------------------------------------------------------------------------
// Set up OC3 and OC4 to do old fashioned Hobby RC pulse control.
// The pulse train is somewhat arbitrary in frequency, only pulse length counts.
// 0-100% is represented by pulse length 1ms to 2ms, and 50% is 1.5ms.
// We use the same timer (TMR2) as the LED PWM functions.

void engine_Initialize() {

	OC3CONbits.OCSIDL = 0; // Stop in idle mode.
	OC3CONbits.OCM = 6; // PWM mode.

	OC4CONbits.OCSIDL = 0; // Stop in idle mode.
	OC4CONbits.OCM = 6; // PWM mode.

	engine_SetGear( 0 ); // Gear to neutral.
	engine_SetThrottle( 0 ); // Throttle to idle.

	hw_WritePort( hw_SWITCH3, 0 );
	hw_OutputPort( hw_SWITCH3 ); // Power to Roboteq.

	engine_LastActuatorUpdate = schedule_time;
}

//---------------------------------------------------------------------------------------------

void engine_ThrottleInitialize() {
	engine_LastJoystickLevel = engine_UNKNOWN_JOYSTICK;
	engine_CurrentGear = engine_UNKNOWN_GEAR;
}

//---------------------------------------------------------------------------------------------
// Read throttle settings and return True if there was any activity.

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
	if( diff < 0 ) diff = - diff;
	if( diff < engine_THROTTLE_MIN_CHANGE ) return 0;
	engine_LastJoystickLevel = engine_Joystick_Level;

	// Scale A/D Converted level down to -100 --  +100 integer, calibrated.

	calibratedLevel = hw_Config->engine_Calibration[p_JoystickMid];
	calibratedLevel = engine_Joystick_Level - calibratedLevel;

	if( calibratedLevel < engine_IDLE_DEADBAND && ( -calibratedLevel < engine_IDLE_DEADBAND ) ) {
		engine_Throttle = 0;
	}
	else if( calibratedLevel > 0 ) {
		engine_Throttle = 100.0 * (float) ( calibratedLevel - engine_IDLE_DEADBAND ) / (float) ( hw_Config->engine_Calibration[p_JoystickMax]
				- hw_Config->engine_Calibration[p_JoystickMid] );
	}
	else {
		engine_Throttle = 100.0 * (float) ( calibratedLevel + engine_IDLE_DEADBAND ) / (float) ( hw_Config->engine_Calibration[p_JoystickMid]
				- hw_Config->engine_Calibration[p_JoystickMin] );
	}

	if( engine_Throttle > 100 ) engine_Throttle = 100;
	if( engine_Throttle < -100 ) engine_Throttle = -100;

	return 1;
}

void engine_UpdateActuators() {

	hw_WritePort( hw_SWITCH3, 1 );

	if( engine_ThrottlePW != engine_CurThrottlePW ) {
		OC3RS = engine_ThrottlePW;
		engine_CurThrottlePW = engine_ThrottlePW;
	}

	if( engine_GearPW != engine_CurGearPW ) {
		OC4RS = engine_GearPW;
		engine_CurGearPW = engine_GearPW;
	}
	engine_LastActuatorUpdate = schedule_time;
}


//---------------------------------------------------------------------------------------------
// Request a throttle level from 0 to 100 and a gear setting.
// The actual gear will not be set until engine RPM allows it.

void engine_RequestThrottleAndGear( char throttle, char gear ) {

	short diff;

	engine_TargetGear = gear;
	engine_TargetThrottle = throttle;

	// engine_ThrottleTimeSteps maintains a simulated RPM counter.
	// For 3000rpm = 3sek and 100ms task interval use / 3:

	// Changing throttle in same gear.

	if( engine_TargetGear == engine_CurrentGear ) {
		diff = engine_Throttle - engine_TargetThrottle;
		if( diff < 0 ) diff = -diff;
		if( engine_GearTimeSteps == 0) {
			engine_SetThrottle( throttle );
		}
		else {
			engine_TargetThrottle = throttle;
		}
	}

	// Putting into gear from idle.

	else if( engine_CurrentGear == 0 && engine_TargetGear != 0 ) {
		diff = 0;
		engine_SetGear( gear );
		engine_GearTimeSteps += engine_GEARBOX_DELAY;
	}

	// Taking out of gear.

	else if( engine_CurrentGear != 0 && engine_TargetGear == 0 ) {
		diff = engine_Throttle;
		engine_SetThrottle( 0 );
	}

	// Reversing.

	else {
		diff = engine_Throttle;
		engine_SetThrottle( 0 );
	}

	engine_ThrottleTimeSteps += diff / 3;
}


//---------------------------------------------------------------------------------------------
// Actually set throttle to a level from 0 to 100.

void engine_SetThrottle(unsigned char level) {
	float fLevel;
	float range;

	engine_Throttle = level;

	fLevel = ( (float) level ) / 100.0;

	range = hw_Config->engine_Calibration[p_ThrottleMax] - hw_Config->engine_Calibration[p_ThrottleMin];
	fLevel = fLevel * range;

	engine_ThrottlePW = hw_Config->engine_Calibration[p_ThrottleMin] + (short) fLevel;
	engine_UpdateActuators();
}

//---------------------------------------------------------------------------------------------
// Actually set gear.

void engine_SetGear(char direction) {

	switch( direction ) {
		case 0: {
			engine_GearPW = hw_Config->engine_Calibration[p_GearNeutral];
			break;
		}
		case 1: {
			engine_GearPW = hw_Config->engine_Calibration[p_GearForward];
			break;
		}
		case -1: {
			engine_GearPW = hw_Config->engine_Calibration[p_GearReverse];
			break;
		}
	}

	// Now do the gear change.

	engine_UpdateActuators();
	engine_CurrentGear = direction;
	engine_LastGearTime = schedule_time;
}

//--------------------------------------------------------------------------------------------
// Engine Task:
// Fade approximated engine RPM towards current throttle setting.
// If a gear change has been requested, do it when the target RPM has been  reached.

void engine_ActuatorTask() {
	long interval;

	if( engine_ThrottleTimeSteps > 0 ) {
		engine_ThrottleTimeSteps--;
	}

	// When putting engine in gear the throttle needs to wait for the longer travel of the
	// gear actuator.

	if( engine_GearTimeSteps > 0 ) {
		engine_GearTimeSteps--;
		if( engine_GearTimeSteps == 0)
			engine_SetThrottle( engine_TargetThrottle );
	}

	// Any gear change pending?

	if( engine_ThrottleTimeSteps == 0 ) {
		if( engine_CurrentGear != engine_TargetGear ) {

			engine_GearTimeSteps += engine_GEARBOX_DELAY;

			// Extra delay if we go straight between fwd and rev.

			if( engine_CurrentGear != 0 && engine_TargetGear != 0 ) {
				engine_GearTimeSteps += engine_GEARBOX_DELAY;
			}

			engine_SetGear( engine_TargetGear );
		}
	}

	// Turn off actuators after a timeout period.
	// There is a small delay turning them on, but this is preferred
	// over having the actuator burn down from overload by trying to counteract
	// engine vibrations for an extended time.

	if( engine_LastActuatorUpdate == 0 ) return;

	if( schedule_time > engine_LastActuatorUpdate ) {
		interval = schedule_time - engine_LastActuatorUpdate;
		if( interval > hw_Config->engine_Calibration[p_ActuatorsTimeout] ) {
			hw_WritePort( hw_SWITCH3, 0 );
			engine_LastActuatorUpdate = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------
// This task reads the joystick and sends it out as a new throttle level if it has changed.

void engine_JoystickTask() {
	event_t event;

	if( engine_CurMasterDevice != hw_DeviceID ) return;

	// Check that this appliance is configured as a throttle sender.

	if( config_GetGroupIdForPort( hw_ANALOG ) == config_GROUP_BROADCAST ) return;

	if( engine_ReadThrottleLevel() ) {

		// Don't send anything if the change was only 1.

		short diff = engine_Throttle - engine_LastThrottle;
		if( diff < 0 ) diff = -diff;
		if( diff < 2 ) return;
		engine_LastThrottle = engine_Throttle;

		// Only send wake-up if there has been a significant pause since last event.

		if( schedule_time - engine_LastGearTime > schedule_SECOND / 2 ) {
			nmea_Wakeup();
		}

		event.PGN = 0;
		event.groupId = config_GetGroupIdForPort( hw_ANALOG );
		event.ctrlDev = hw_DeviceID;
		event.ctrlPort = hw_ANALOG;
		event.ctrlEvent = e_THROTTLE_CHANGE;
		event.data = engine_Throttle;
		event.info = engine_Joystick_Level;
		nmea_SendEvent( &event );
		engine_LastGearTime = schedule_time;
	}
}

//--------------------------------------------------------------------------------------------
// Menu state machine handlers for engine settings:
// engine_ThrottleMonitor - Set up the display for monitoring throttle levels.
// engine_ThrottleMonitorUpdater - Continuously running task that updates the display.
//

int engine_ThrottleMonitor() {
	display_SetPosition( 1, 2 );
	display_Write( "Throttle:" );
	display_SetPosition( 1, 3 );
	display_Write( "Gearbox:" );

	schedule_AddTask( engine_ThrottleMonitorUpdater, schedule_SECOND / 2 );
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

	display_NumberFormat( numberString, 4, engine_LastJoystickLevel );
	display_SetPosition( 17, 1 );
	display_Write( numberString );

	// Now show a graphical representation of the resulting actuator positions.

	gear = 0;
	throttle = engine_Throttle;
	if( throttle > 0 ) gear = 1;
	if( throttle < 0 ) {
		gear = -1;
		throttle = -throttle;
	}

	throttle = throttle / 2; // Scale from 0-100 to 0-50

	display_HorizontalBar( 10, 2, throttle );

	switch( gear ) {
		case 0: {
			gear = 25;
			break;
		}
		case 1: {
			gear = 49;
			break;
		}
		case -1: {
			gear = 1;
			break;
		}
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

int engine_ProcessEvent(event_t *event, unsigned char port, unsigned char action) {
	event_t masterEvent;
	char throttle;

	if( !hw_Actuators_Installed ) return 0;

	switch( event->ctrlEvent ) {

		case e_THROTTLE_CHANGE: {

			throttle = event->data; // Make a signed value from event->data which is unsigned.

			if( event->ctrlDev != engine_CurMasterDevice ) break;

			if( throttle == 0 ) {
				engine_RequestThrottleAndGear( 0, 0 );
			}

			else if( throttle > 0 ) {
				engine_RequestThrottleAndGear( throttle, 1 );
			}

			else {
				engine_RequestThrottleAndGear( -throttle, -1 );
			}

			break;
		}

		case e_KEY_CLICKED: {

			// Clicking on an actuator means a new device becomes the master.

			if( engine_CurMasterDevice != event->ctrlDev ) {
				engine_CurMasterDevice = event->ctrlDev;
				masterEvent.info = engine_CurMasterDevice;
				masterEvent.data = event->data; // Return info about what key was clicked to become master.

				// Turn on power to the Roboteq.

				hw_WritePort( hw_SWITCH1, 1 );
				hw_WritePort( hw_SWITCH3, 1 );

			}

			// The current master want to shut down.

			else {

				engine_CurMasterDevice = 0;

				engine_RequestThrottleAndGear( 0, 0 );

				masterEvent.info = 0;
				masterEvent.data = 0;

				// Wait a while for actuators to settle, then turn off power to the Roboteq.

				schedule_Sleep( schedule_SECOND * 2 );
				hw_WritePort( hw_SWITCH1, 0 );
				hw_WritePort( hw_SWITCH3, 0 );
			}

			nmea_Wakeup();

			masterEvent.ctrlEvent = e_THROTTLE_MASTER;
			masterEvent.ctrlPort = port;
			masterEvent.ctrlDev = hw_DeviceID;
			masterEvent.groupId = config_GetGroupIdForPort( port );

			nmea_SendEvent( &masterEvent );

			break;
		}
	}
	return 0;
}

//--------------------------------------------------------------------------------------------

void engine_SetMaster(event_t *event) {
	unsigned char port;
	unsigned char controlGroup;

	// Finding what key on my device that is configured to control throttle mastering.
	// This algorithm has dependency on keypad ports to come before analog inputs.

	engine_CurMasterDevice = event->info;

	controlGroup = config_GetControllingGroup( event->groupId );

	for( port = 0; port < hw_PortCount; port++ ) {

		if( config_GetGroupIdForPort( port ) == controlGroup ) {

			if( engine_CurMasterDevice == hw_DeviceID ) {
				hw_AcknowledgeSwitch( port, 1 );

				// Turn on joystick power
				hw_WritePort( hw_SWITCH1, 1 );
			}
			else {
				hw_AcknowledgeSwitch( port, 0 );
				hw_WritePort( hw_SWITCH1, 0 );
			}
		}
	}


	// Our device lost master status. Our device doesn't necessarily use the same button
	// to request master status as the new master, so we need to go through
	// our config to find which indicator to turn off.

}
