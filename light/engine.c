/*
 * engine.c
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#include "hw.h"

unsigned short		engine_ThrottleCalibration[3] = { 20, 500, 1000 };
unsigned char		engine_ThrottleSetting;
signed char 		engine_GearboxSetting;
unsigned char		engine_CurThrottle;
signed char			engine_CurGearbox;
unsigned char		engine_CurMasterDevice;

void engine_ReadJoystickLevel() {

	unsigned short level;

	level = ADC_Read( 4 ); // XXX Make configurable.

	if( engine_CurMasterDevice != hw_DeviceID ) {

		// XXX Need to become Master Controller before update!
		engine_CurMasterDevice = hw_DeviceID;
	}

	else {

		// Scale A/D Concerted level down to 1 0-99 integer, calibrated to the
		// individual joystick.

		unsigned short range = engine_ThrottleCalibration[2] - engine_ThrottleCalibration[0];

		float floatlevel = (float)level;
		floatlevel -= engine_ThrottleCalibration[1];

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
}

void engine_UpdateActuators() {
	if( (engine_ThrottleSetting == engine_CurThrottle) &&
		(engine_GearboxSetting == engine_CurGearbox) ) return;
}
