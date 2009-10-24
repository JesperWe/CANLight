/*
 * engine.h
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#ifndef ENGINE_H_
#define ENGINE_H_

enum engine_CalibrationParameters {
	/* 00 */ p_None,
	/* 01 */ p_ThrottleMin,
	/* 02 */ p_ThrottleMax,
	/* 03 */ p_GearNeutral,
	/* 04 */ p_GearForward,
	/* 05 */ p_GearReverse,
	/* 06 */ p_NO_CALIBRATION_PARAMS
};

extern short	engine_Calibration[];
extern short	engine_ThrottleSetting;
extern short 	engine_GearboxSetting;
extern short	engine_CurThrottle;
extern short	engine_CurGearbox;
extern unsigned char	engine_CurMasterDevice;

void engine_Initialize();
void engine_ReadJoystickLevel();
void engine_UpdateActuators();
void engine_SetThrottle( unsigned char level );
void engine_SetGear( char direction );

#endif /* ENGINE_H_ */
