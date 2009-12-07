/*
 * engine.c
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#include "hw.h"
#include "events.h"
#include "nmea.h"
#include "engine.h"

short	engine_Calibration[p_NO_CALIBRATION_PARAMS];

short	engine_ThrottlePW;
short	engine_GearPW;
short	engine_CurThrottlePW;
short	engine_CurGearPW;

unsigned char	engine_CurMasterDevice;
short	engine_LastJoystickLevel;
short 	engine_GearSwitchTime;
short 	engine_CurrentRPM;
short 	engine_TargetRPM;

unsigned char	engine_TargetThrottle;

char	engine_CurrentGear;
char	engine_TargetGear;

//---------------------------------------------------------------------------------------------
// Set up TIMER3 and TIMER4 to do old fashioned Hobby RC pulse control.
// The pulse train is somewhat arbitrary in frequency, only pulse length counts.
// 0-100% is represented by pulse length 1ms to 2ms, and 50% is 1.5ms.

void engine_Initialize() {
	int i;

	OC3CONbits.OCSIDL = 1; 		// Stop in idle mode.
	OC3CONbits.OCM = 6; 		// PWM mode.

	OC4CONbits.OCSIDL = 1; 		// Stop in idle mode.
	OC4CONbits.OCM = 6; 		// PWM mode.

	TRISCbits.TRISC2 = 0;
	TRISCbits.TRISC0 = 0;

	// Restore actuator calibrations from Flash memory.

	for( i=0; i<p_NO_CALIBRATION_PARAMS; i++ ) {
		engine_Calibration[i] = hw_Config.engine_Calibration[i];
	}

	engine_LastJoystickLevel = engine_UNKNOWN_JOYSTICK;
	engine_CurrentGear = engine_UNKNOWN_GEAR;

	engine_SetGear(0);		// Gear to neutral.
	engine_SetThrottle(0);	// Throttle to idle.
}


void engine_ReadJoystickLevel() {

	short level, diff, gear;
	unsigned char throttle;
	static unsigned char lastThrottle = 0;
	event_t event;
	float floatlevel;

	level = ADC_Read( 4 ); // XXX Make configurable.

	// First time since device reset?

	if( engine_LastJoystickLevel == engine_UNKNOWN_JOYSTICK ) {
		engine_LastJoystickLevel = level;
		return;
	}

	// Require a significant movement before taking action.

	diff = level - engine_LastJoystickLevel;
	if( diff<3 && diff>-3 ) return;
	engine_LastJoystickLevel = level;

	if( engine_CurMasterDevice != hw_DeviceID ) {

		// Need to become Master Controller before update!

		event.PGN = 0;
		event.atTimer = 0;
		event.ctrlDev = hw_DeviceID;
		event.ctrlFunc = 0;
		event.ctrlEvent = e_THROTTLE_MASTER;
		event.data = 0;

		nmea_SendEvent( &event );

		engine_CurMasterDevice = hw_DeviceID;
	}

	// Scale A/D Concerted level down to 1 0-99 integer, calibrated to the
	// individual joystick.

	unsigned short range = engine_Calibration[ p_JoystickMax ] - engine_Calibration[ p_JoystickMin ];

	floatlevel = (float)level;
	floatlevel -= engine_Calibration[ p_JoystickMid ];

	floatlevel = 2*floatlevel / (float)range;
	if( floatlevel < -1.0 ) floatlevel = -1.0;
	if( floatlevel > 1.0 ) floatlevel = 1.0;

	// Gear forward/reverse.

	gear = 1;
	if( floatlevel < 0.0 ) {
		gear = -1;
		floatlevel = -floatlevel;
	}

	if( floatlevel < 0.08 ) { // XXX Idling dead-band. Make configurable.
		gear = 0;
		floatlevel = 0;
	}

	// Decrease sensitivity near center.

	floatlevel = floatlevel * (1.0 + floatlevel) / 2.0;
	throttle = (unsigned char)(floatlevel * 100.0);

	if( throttle == lastThrottle ) return;
	lastThrottle = throttle;

	// Send Throttle and Gear settings on bus.

	event.PGN = 0;
	event.atTimer = 0;
	event.ctrlDev = hw_DeviceID;
	event.ctrlFunc = gear;
	event.ctrlEvent = e_SET_THROTTLE;
	event.data = throttle;
	event.atTimer = hw_HeartbeatCounter;
	nmea_SendEvent( &event );
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

	range = engine_Calibration[ p_ThrottleMax ] - engine_Calibration[ p_ThrottleMin ];
	fLevel = fLevel * (float)range;

	engine_ThrottlePW = engine_Calibration[ p_ThrottleMin ] + (short)fLevel;
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
// Actually set gear. Possible values are -1, 0, +1.

void engine_SetGear( char direction ) {

	switch( direction ) {
		case 0:  { engine_GearPW = engine_Calibration[ p_GearNeutral ]; break; }
		case 1:  { engine_GearPW = engine_Calibration[ p_GearForward ]; break; }
		case -1: { engine_GearPW = engine_Calibration[ p_GearReverse ]; break; }
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

void engine_Task( void *pvParameters )
{
	if( engine_TargetRPM > engine_CurrentRPM ) engine_CurrentRPM++;
	if( engine_TargetRPM < engine_CurrentRPM ) engine_CurrentRPM--;

	// Any gear change pending?

	if( engine_CurrentGear != engine_TargetGear ) {
		if( engine_CurrentRPM == engine_TargetRPM ) {

			// Require at least 1 seconds between gear changes.

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

