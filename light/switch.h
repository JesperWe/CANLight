/*
 * switch.h
 *
 *  Created on: 30 sep 2009
 *      Author: Jesper W
 */

#ifndef SWITCH_H_
#define SWITCH_H_

#include "hw.h"
#include "led.h"
#include "events.h"

void switch_ProcessEvent( event_t *event, unsigned char function );

#endif /* SWITCH_H_ */
