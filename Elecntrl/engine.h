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
#define engine_IDLE_DEADBAND 		8
#define engine_THROTTLE_MIN_CHANGE 4
#define engine_GEARBOX_DELAY		10 // Number of ActuatorTask cycles.

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
	/* 09 */ p_ActuatorsTimeout,
	/* 10 */ engine_NO_CALIBRATION_PARAMS
};

extern short	engine_Calibration[];

extern short	engine_ThrottlePW;
extern short 	engine_GearPW;
extern short	engine_CurThrottlePW;
extern short	engine_CurGearPW;

extern short			engine_LastJoystickLevel;
extern unsigned char	engine_CurMasterDevice;
extern short 			engine_CurrentRPM;
extern short 			engine_CurrentRPM;
extern unsigned long	engine_LastGearTime;
extern unsigned long engine_LastActuatorUpdate;
extern unsigned char	engine_TargetThrottle;
extern short			engine_ThrottleTimeSteps;
extern short			engine_GearTimeSteps;

extern char			engine_CurrentGear;
extern char			engine_TargetGear;

extern short			engine_Joystick_Level;
extern short			engine_Gear;
extern char			engine_Throttle;
extern char			engine_LastThrottle;

extern unsigned char 	engine_JoystickCalibrationMonitor;


void engine_Initialize();
void engine_ThrottleInitialize();
unsigned char engine_ReadThrottleLevel();
void engine_UpdateActuators();

void engine_RequestGear( char direction );
void engine_RequestThrottleAndGear( char throttle, char gear );

void engine_SetThrottle( unsigned char level );
void engine_SetGear( char direction );

void engine_InterruptService();

void engine_ActuatorTask();
void engine_JoystickTask();

int engine_ThrottleMonitor();
void engine_ThrottleMonitorUpdater();

int engine_CalibrationParams();

int engine_ProcessEvent( event_t *event, unsigned char function, unsigned char action );

void engine_SetMaster( event_t *event );

#endif /* ENGINE_H_ */
