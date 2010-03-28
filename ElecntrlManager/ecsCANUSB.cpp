/*
 * $Revision$
 * $Author$
 * $Date$
 */

#include <cstdio>
#include "ecsCANUSB.h"
#include "ecsManagerApp.h"

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

//------------------------------------------------------------------------------------

int ecsCANUSB::status() {
	return lastStatus;

}

//------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------

int ecsCANUSB::close() {
	lastStatus = BusDisconnected;
	return canusb_Close( adapterHandle );
}

//------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------
// The read callback function called by the device driver when data is read from the bus.
// This is a "C" style callback function which cannot be part of an object context.

void __stdcall readCallbackFn( CANMsg* msg ) {
	int i;
	QString line = "";
	QString buf;

	line += (msg->flags & CANMSG_EXTENDED)   ?   "E" : "S";
	line += (msg->flags & CANMSG_RTR)   ?   "R" : " ";
	line += buf.sprintf( " %08X time=%08X len=%d",
			 (unsigned int)(msg->id),
			 (unsigned int)(msg->timestamp),
			 msg->len );

	if( msg->len ) {
		line += " [ ";
		for ( i=0; i<msg->len; i++ ) {
			line += buf.sprintf( "%02X ", msg->data[ i ] );
		}
		line += "]";
	}

	ecsManagerApp::inst()->logWidget->appendPlainText( line );
}

//------------------------------------------------------------------------------------

void ecsCANUSB::registerReader() {
	canusb_setReceiveCallBack( adapterHandle, (LPFNDLL_RECEIVE_CALLBACK)readCallbackFn );
}

//------------------------------------------------------------------------------------

void ecsCANUSB::unregisterReader() {
	canusb_setReceiveCallBack( adapterHandle, NULL );
}
