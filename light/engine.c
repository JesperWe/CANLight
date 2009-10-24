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
short	engine_ThrottleSetting;
short	engine_GearboxSetting;
short	engine_CurThrottle;
short	engine_CurGearbox;
unsigned char	engine_CurMasterDevice;


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

	engine_SetGear(0);		// Gear to neutral.
	engine_SetThrottle(0);	// Throttle to idle.
}


void engine_ReadJoystickLevel() {

	unsigned short level;
	event_t event;

	level = ADC_Read( 4 ); // XXX Make configurable.

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

	else {

		// Scale A/D Concerted level down to 1 0-99 integer, calibrated to the
		// individual joystick.

		unsigned short range = 1000; // XXX Add calibration

		float floatlevel = (float)level;
		//floatlevel -= engine_ThrottleCalibration[1];

		floatlevel = 2*floatlevel / (float)range;
		if( floatlevel < -1.0 ) floatlevel = -1.0;
		if( floatlevel > 1.0 ) floatlevel = 1.0;

		// Gear forward/reverse.

		engine_GearboxSetting = 50;
		if( floatlevel < 0.0 ) {
			engine_GearboxSetting = -50;
			floatlevel = -floatlevel;
		}

		if( floatlevel < 0.08 ) { // Idling dead-band.
			engine_GearboxSetting = 0;
			floatlevel = 0;
		}
		
		// Decrease sensitivity near center.

		floatlevel = floatlevel * (1.0 + floatlevel) / 2.0;

		engine_ThrottleSetting = (unsigned char)(floatlevel * 100.0);
	}

	// Don't generate events unless the joystick has moved significantly.

	if( engine_ThrottleSetting > engine_CurThrottle )
		level = engine_ThrottleSetting - engine_CurThrottle;
	else
		level = engine_CurThrottle - engine_ThrottleSetting;

	if( level > 3 ) {

		engine_CurThrottle = engine_ThrottleSetting;

		event.PGN = 0;
		event.atTimer = 0;
		event.ctrlDev = hw_DeviceID;
		event.ctrlFunc = engine_GearboxSetting;
		event.ctrlEvent = e_SET_THROTTLE;
		event.data = engine_ThrottleSetting;

		nmea_SendEvent( &event );
	}

}


void engine_UpdateActuators() {

	if( engine_ThrottleSetting != engine_CurThrottle ) {
		OC3RS = engine_ThrottleSetting;
		engine_CurThrottle = engine_ThrottleSetting;
	}

	if (engine_GearboxSetting != engine_CurGearbox ) {
		OC4RS = engine_GearboxSetting;
		engine_CurGearbox = engine_GearboxSetting;
	}
}


//---------------------------------------------------------------------------------------------
// Set throttle to a level from 0 to 100.

void engine_SetThrottle( unsigned char level ) {
	float fLevel = ((float)level) / 100.0;
	short range = engine_Calibration[ p_ThrottleMax ] - engine_Calibration[ p_ThrottleMin ];
	fLevel = fLevel * (float)range;
	engine_ThrottleSetting = engine_Calibration[ p_ThrottleMin ] + (short)fLevel;
	engine_UpdateActuators();
}


//---------------------------------------------------------------------------------------------
// Set gear. Possible values are -1, 0, +1.

void engine_SetGear( char direction ) {
	switch( direction ) {
		case 0:  { engine_GearboxSetting = engine_Calibration[ p_GearNeutral ]; break; }
		case 1:  { engine_GearboxSetting = engine_Calibration[ p_GearForward ]; break; }
		case -1: { engine_GearboxSetting = engine_Calibration[ p_GearReverse ]; break; }
	}
	engine_UpdateActuators();
}

