/*
 * $Revision$
 * $Author$
 * $Date$
 */

#include <cstdio>
#include <time.h>
#include "ecsCANUSB.h"
#include "ecsManagerApp.h"
#include "nmea.h"

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

QString ecsCANUSB::errorString( int result ) {
	switch( result ) {
	case 1: { return "Success"; }
	case -1: { return "General Error"; }
	case -2: { return "Error Opening Subsystem"; }
	case -3: { return "Error in Command Subsystem"; }
	case -4: { return "Device Not Open"; }
	case -5: { return "Transmit Queue Full"; }
	case -6: { return "Invalid Parameter"; }
	case -7: { return "No Message"; }
	case -8: { return "Memory Error"; }
	case -9: { return "No Device"; }
	case -10: { return "Timeout"; }
	case -11: { return "Invalid Hardware"; }
	default: { return "Unrecognized Error Code"; }
	}
}

//------------------------------------------------------------------------------------

void ecsCANUSB::alert( QString location, int result ) {
	QString msg = location;
	msg.append( errorString( result) );
	msg.append( " (" );
	msg.append( QString::number(result) );
	msg.append(")");

	QMessageBox msgBox;
	msgBox.setText( msg );
	msgBox.setIcon( QMessageBox::Critical );
	msgBox.exec();
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
		canusb_SetTimeouts( adapterHandle, 2000, 2000 );
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

//------------------------------------------------------------------------------------
// Build NMEA PGN

void nmea_MakePGN( nmea_PGN_t &outPGN,
	unsigned short pgn_priority,
	unsigned short pgn_no,
	unsigned short msg_bytes ) {

	outPGN.PGN				= pgn_no;
	outPGN.PDUFormat 		= (pgn_no & 0xFF00) >> 8;
	outPGN.PDUSpecific 		= pgn_no & 0x00FF;
	outPGN.Datapage			= 0;
	outPGN._Reserved		= 0;
	outPGN.Priority			= pgn_priority;
}


void ecsCANUSB::sendConfig( QByteArray &configFile ) {

	nmea_PGN_t outPGN;
	CANMsg msg;
	int msgLength;
	int noPackets;
	int sentBytes;
	int result;

	msgLength = configFile.length();
	noPackets = msgLength / 7;
	if( msgLength % 7 ) noPackets++;

	// First tell the bus we are about to transmit a multi-packet message.

	nmea_MakePGN( outPGN, 1, nmea_CM_BAM, 8 );

	msg.id = outPGN.PGN;
	msg.flags = CANMSG_EXTENDED;
	msg.timestamp = time( 0 );
	msg.len = 8;

	msg.data[0] = 0x20; // TP.CM_BAM
	msg.data[1] = (msgLength & 0xFF00) >> 8;
	msg.data[2] = msgLength & 0x00FF;
	msg.data[3] = noPackets;
	msg.data[4] = 0xFF; // Reserved
	msg.data[5] = (nmea_LIGHTING_DATA & 0xFF0000) >> 16;
	msg.data[6] = (nmea_LIGHTING_DATA & 0x00FF00) >> 8;
	msg.data[7] =  nmea_LIGHTING_DATA & 0x0000FF;

	result = canusb_Write( adapterHandle, &msg );
	if( result <= 0 ) {
		alert( "NMEA Error for Multi-packet Header: ", result );
		return;
	}

	// Now do data packets.

	nmea_MakePGN( outPGN, 1, nmea_DATATRANSFER, 8 );
	msg.id = outPGN.PGN;
	sentBytes = 0;

	for( int i=0; i<noPackets; i++ ) {

		msg.timestamp = time( 0 );
		msg.data[0] = noPackets;

		for( int j=1; j<8; j++ ) {

			// Last data packet might have less bytes.

			if( sentBytes >= msgLength ) {
				msg.len = j;
				break;
			}
			msg.data[j] = configFile[sentBytes++];
		}

		result = canusb_Write( adapterHandle, &msg );
		if( result <= 0 ) {
			QString location = "NMEA Error on packet ";
			location.append( QString::number(i) );
			location.append( ": " );
			alert( location, result );
			return;
		}
	}

}
