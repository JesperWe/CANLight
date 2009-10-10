/*
 * engine.h
 *
 *  Created on: 10 okt 2009
 *      Author: Jesper W
 */

#ifndef ENGINE_H_
#define ENGINE_H_

extern unsigned short		engine_ThrottleCalibration[];
extern unsigned char		engine_ThrottleSetting;
extern signed char 		engine_GearboxSetting;
extern unsigned char		engine_CurThrottle;
extern signed char		engine_CurGearbox;
extern unsigned char		engine_CurMasterDevice;

void engine_ReadJoystickLevel();
void engine_UpdateActuators();

#endif /* ENGINE_H_ */
