/*
 * ballast.c
 *
 *  Created on: 201-jun-27
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "nmea.h"

//---------------------------------------------------------------------------------------------
// The defined states the ballast tanks can be in are:
//
// 0 - Idle. Tanks are empty and all valves are closed.
// 1 - Empty port tank. Outlet and port vales open, rest closed.
// 2 - Empty stbd tank. Outlet and stbd vales open, rest closed.
// 3 - Pump port. Inlet and port valves open. Pump running.
// 4 - Pump port. Inlet and stbd valves open. Pump running.
// 5 - Connect. Port and stbd valves open. Pump is off.
// 17 - Emergency bilge. Outlet open and pump running.

unsigned char ballast_State = 0;

void ballast_KeyClick( unsigned char key, unsigned char event ) {
	unsigned char previousState = ballast_State;

	switch( event ) {
		case e_KEY_CLICKED: {
			if( ballast_State == 0 ) { ballast_State = 5; break; }
			if( ballast_State == 5 ) { ballast_State = 0; break; }
			if( ballast_State == 17 ) { ballast_State = 0; break; }
			if( ballast_State == 1 && key == 1 ) { ballast_State = 0; break; }
			if( ballast_State == 2 && key == 2 ) { ballast_State = 0; break; }
			if( ballast_State == 3 && key == 1 ) { ballast_State = 0; break; }
			if( ballast_State == 4 && key == 2 ) { ballast_State = 0; break; }
			break;
		}
		case e_KEY_DOUBLECLICKED: {
			if( ballast_State == 0 && key == 1 ) { ballast_State = 1; break; }
			if( ballast_State == 0 && key == 2 ) { ballast_State = 2; break; }
			break;
		}
		case e_KEY_TRIPLECLICKED: {
			if( ballast_State == 0 && key == 1 ) { ballast_State = 3; break; }
			if( ballast_State == 0 && key == 2 ) { ballast_State = 4; break; }
			break;
		}
		case e_KEY_HOLDING: {
			if( ballast_State == 0 ) { ballast_State = 17; break; }
			break;
		}
	}

	if( ballast_State != previousState ) {
		event_t event;
		event.PGN = 0;
		event.groupId = config_GROUP_BROADCAST;
		event.ctrlDev = hw_DeviceID;
		event.ctrlPort = hw_UNKNOWN;
		event.ctrlEvent = e_BALLAST_STATE;
		event.data = ballast_State;
		event.info = 0;
		nmea_SendEvent( &event );

		hw_WritePort( hw_LED2, 0 );
		hw_WritePort( hw_LED3, 0 );

		if( ballast_State == 1 || ballast_State == 3 ||
			ballast_State == 5 || ballast_State == 17 )
			hw_WritePort( hw_LED2, 1 );

		if( ballast_State == 2 || ballast_State == 4 ||
			ballast_State == 5 || ballast_State == 17 )
			hw_WritePort( hw_LED3, 1 );
	}
}
