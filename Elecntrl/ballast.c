/*
 * ballast.c
 *
 *  Created on: 201-jun-27
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "nmea.h"
#include "schedule.h"
#include "ballast.h"

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
unsigned char ballast_Transitioning = 0;

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
		unsigned char status;
		event_t event;
		event.PGN = 0;
		event.groupId = config_GROUP_BROADCAST;
		event.ctrlDev = hw_DeviceID;
		event.ctrlPort = hw_UNKNOWN;
		event.ctrlEvent = e_BALLAST_STATE;
		event.data = ballast_State;
		event.info = 0;

		nmea_Wakeup();
		status = nmea_SendEvent( &event );

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


//---------------------------------------------------------------------------------------------
// This stuff is hard coded. Currently it is not worth the effort to make it configurable.
// An and Bn variables correspond to the output switches on device 20 and 21.
// The pump is on device 9.
//
// Output Valve: Open=A4+A3, Close=A4
// Inlet Valve: Open=A2+A1, Close=A2
// Port Valve: Open=B1+B3, Close=B1
// Stbd Valve: Open=B4+B2, Close=B4

void ballast_GoToState( unsigned char state ) {
	unsigned char A1,A2,A3,A4,B1,B2,B3,B4,pump;

	if( hw_DeviceID != 9 &&
		hw_DeviceID != 21 &&
		hw_DeviceID != 22 ) return;

	switch( state ) {
		case 0: {
			A1=0; A2=1; A3=0; A4=1;
			B1=1; B2=0; B3=0; B4=1;
			pump=0;
			break;
		}
		case 1: {
			A1=0; A2=1; A3=1; A4=1;
			B1=1; B2=0; B3=1; B4=1;
			pump=0;
			break;
		}
		case 2: {
			A1=0; A2=1; A3=1; A4=1;
			B1=1; B2=1; B3=0; B4=1;
			pump=0;
			break;
		}
		case 3: {
			A1=1; A2=1; A3=0; A4=1;
			B1=1; B2=0; B3=1; B4=1;
			pump=1;
			break;
		}
		case 4: {
			A1=1; A2=1; A3=0; A4=1;
			B1=1; B2=1; B3=0; B4=1;
			pump=1;
			break;
		}
		case 5: {
			A1=0; A2=1; A3=0; A4=1;
			B1=1; B2=1; B3=1; B4=1;
			pump=0;
			break;
		}
		case 17: {
			A1=0; A2=1; A3=1; A4=1;
			B1=1; B2=0; B3=0; B4=1;
			pump=1;
			break;
		}
	}

	if( hw_DeviceID == 9 ) {
		hw_WritePort( hw_SWITCH3, pump );
	}

	if( hw_DeviceID == 21 ) {
		hw_WritePort( hw_SWITCH1, A1 );
		hw_WritePort( hw_SWITCH2, A2 );
		hw_WritePort( hw_SWITCH3, A3 );
		hw_WritePort( hw_SWITCH4, A4 );
	}

	if( hw_DeviceID == 22 ) {
		hw_WritePort( hw_SWITCH1, B1 );
		hw_WritePort( hw_SWITCH2, B2 );
		hw_WritePort( hw_SWITCH3, B3 );
		hw_WritePort( hw_SWITCH4, B4 );
	}

	// Wait 10 seconds before disengaging.

	ballast_Transitioning = 15;
}


//---------------------------------------------------------------------------------------------

void ballast_ShutOffTask() {

	if( hw_DeviceID != 21 &&
		hw_DeviceID != 22 ) return;

	if( ballast_Transitioning == 0xFF ) return;

	ballast_Transitioning--;
	if( ballast_Transitioning > 0 ) return;
	ballast_Transitioning = 0xFF;

	hw_WritePort( hw_SWITCH1, 0 );
	hw_WritePort( hw_SWITCH2, 0 );
	hw_WritePort( hw_SWITCH3, 0 );
	hw_WritePort( hw_SWITCH4, 0 );
}
