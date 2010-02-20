/*
 * $Revision$
 * $Author$
 * $Date$
 * $Id$
 */

#include <cstdio>

#include "ecsCANUSB.h"

ecsCANUSB::ecsCANUSB() {
	int canStatus;
	adapterHandle = 0;
	strcpy( adapterInfo, "[ Not Present ]" );

	canStatus = canusb_getFirstAdapter( &adapterSerial[0], 32 );
	if( canStatus < 0 ) lastStatus = DriverNotInstalled;
	else if( canStatus == 0 ) lastStatus = DongleNotPresent;
	else lastStatus = BusDisconnected;
	return;
}


int ecsCANUSB::status() {
	return lastStatus;
}

int ecsCANUSB::open() {
	adapterHandle = canusb_Open(
			adapterSerial, "250",
			CANUSB_ACCEPTANCE_CODE_ALL,
			CANUSB_ACCEPTANCE_MASK_ALL, 0 );
	if( adapterHandle > 0 ) {
		lastStatus = BusOpen;
		canusb_VersionInfo( adapterHandle, adapterInfo );
		return adapterHandle;
	}
	lastStatus = DeviceError;
	return adapterHandle;
}

int ecsCANUSB::close() {
	lastStatus = BusDisconnected;
	return canusb_Close( adapterHandle );
}

char* ecsCANUSB::info() {
	int resultCode;
	// Get version info from adapter if present but never opened.

	if( ! adapterHandle && ( lastStatus == BusDisconnected ) ) {
		adapterHandle = canusb_Open(
				adapterSerial, "250",
				CANUSB_ACCEPTANCE_CODE_ALL,
				CANUSB_ACCEPTANCE_MASK_ALL, 0 );
		resultCode = canusb_VersionInfo( adapterHandle, adapterInfo );
		if( resultCode < 0 ) {
			sprintf( adapterInfo,  "[ Not available. Error Code %d ]", resultCode );
		}
		canusb_Close( adapterHandle );
	}

	return adapterInfo;
}
