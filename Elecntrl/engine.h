/*
 * engine.h
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#ifndef ENGINE_H_
#define ENGINE_H_


#define engine_UNKNOWN_GEAR		17
#define engine_UNKNOWN_JOYSTICK	4711
#define engine_JOYSTICK_AD_CHANNEL	10


enum engine_CalibrationParameters {
	/* 00 */ p_None,
	/* 01 */ p_ThrottleMin,
	/* 02 */ p_ThrottleMax,
	/* 03 */ p_GearNeutral,
	/* 04 */ p_GearForward,
	/* 05 */ p_GearReverse,
	/* 06 */ p_JoystickMin,
	/* 07 */ p_JoystickMid,
	/* 08 */ p_JoystickMax,
	/* 09 */ p_NO_CALIBRATION_PARAMS
};

extern short	engine_Calibration[];

extern short	engine_ThrottlePW;
extern short 	engine_GearPW;
extern short	engine_CurThrottlePW;
extern short	engine_CurGearPW;

extern short	engine_LastJoystickLevel;
extern unsigned char	engine_CurMasterDevice;
extern short 	engine_CurrentRPM;
extern short 	engine_TargetRPM;
extern short 	engine_GearSwitchTime;
extern unsigned char	engine_TargetThrottle;

extern char	engine_CurrentGear;
extern char	engine_TargetGear;

extern short	engine_Joystick_Level;
extern short	engine_Gear;
extern unsigned char	engine_Throttle;

void engine_Initialize();
void engine_ThrottleInitialize();
unsigned char engine_ReadThrottleLevel();
void engine_UpdateActuators();

void engine_RequestGear( char direction );
void engine_RequestThrottle( unsigned char level );

void engine_SetThrottle( unsigned char level );
void engine_SetGear( char direction );

void engine_InterruptService();

void engine_ActuatorTask();
void engine_JoystickTask();

int engine_ThrottleMonitor();
void engine_ThrottleMonitorUpdater();

#endif /* ENGINE_H_ */
