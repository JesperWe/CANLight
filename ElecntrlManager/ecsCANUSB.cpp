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
// This method is needed to convert the read callback call into a Qt signal, which is thread safe.
// (Callbacks from the device driver comes from a different thread)

void ecsCANUSB::readCallback( const QString &text ) {
	emit addLogLine( text );
}

//------------------------------------------------------------------------------------
// The read callback function called by the device driver when data is read from the bus.
// This is a "C" style callback function which cannot be part of an object context.
//
// NB! That we can't use most Qt features in this code since it is executed outside of the Qt event loop
// and in a separate thread.

void __stdcall readCallbackFn( CANMsg* msg ) {
	int pgn;
	QString line = "";
	QString buf;
	QString eventName;
	QString funcName;
	short data;
	pgn = (msg->id & 0xFFFF00) >> 8;

	line += (msg->flags & CANMSG_EXTENDED)   ?   "E" : "S";
	line += (msg->flags & CANMSG_RTR)   ?   "R" : " ";
	line += buf.sprintf( " %08X (PGN %d): ",
			 (unsigned int)(msg->id),
			 pgn );

	switch( pgn ) {

	case nmea_CM_BAM: {
			line += buf.sprintf( "TransferProtocol %d bytes, %d packages, Contained PGN %d",
									 msg->data[ 1 ]<<8 | msg->data[2],
									 msg->data[ 3 ],
									 msg->data[ 5 ] << 16 | msg->data[ 6 ]<<8 | msg->data[7] );
			break;
		}

	case nmea_DATATRANSFER: {
			line += buf.sprintf( "TP Data Packet %d: %02x %02x %02x %02x %02x %02x %02x",
								 msg->data[ 0 ], msg->data[ 1 ], msg->data[ 2 ], msg->data[ 3 ],
								 msg->data[ 4 ], msg->data[ 5 ], msg->data[ 6 ], msg->data[ 7 ] );
			break;
		}

	case nmea_LIGHTING_COMMAND: {
			data = msg->data[ 1 ] ;

			switch( msg->data[ 5 ] ) {
			case 1: funcName = "hw_CAN_RATE"; break;
			case 2: funcName = "hw_CAN_EN"; break;
			case 3: funcName = "hw_LED_RED"; break;
			case 4: funcName = "hw_LED_WHITE"; break;
			case 5: funcName = "hw_LED1"; break;
			case 6: funcName = "hw_LED2"; break;
			case 7: funcName = "hw_LED3"; break;
			case 8: funcName = "hw_SWITCH1"; break;
			case 9: funcName = "hw_SWITCH2"; break;
			case 10: funcName = "hw_SWITCH3"; break;
			case 11: funcName = "hw_SWITCH4"; break;
			case 12: funcName = "hw_KEY1"; break;
			case 13: funcName = "hw_KEY2"; break;
			case 14: funcName = "hw_KEY3"; break;
			case 15: funcName = "hw_LED_LIGHT"; break;
			case 16: funcName = "hw_BACKLIGHT"; break;
			case 17: {
					funcName = "hw_ANALOG";
					if( data > 127 ) 	data = data - 256; // Convert from unsigned char to signed.
					break;
				}
			case 18: funcName = "hw_DIGITAL_IN"; break;
			case 19: funcName = "hw_PWM1"; break;
			case 20: funcName = "hw_PWM2"; break;
			default: funcName = "<unknown>";
			}

			switch( msg->data[ 6 ] ) {
			case 1: eventName = "e_KEY_CLICKED"; break;
			case 2: eventName = "e_KEY_HOLDING"; break;
			case 3: eventName = "e_KEY_RELEASED"; break;
			case 4: eventName = "e_KEY_DOUBLECLICKED"; break;
			case 5: eventName = "e_KEY_TRIPLECLICKED"; break;
			case 6: eventName = "e_SWITCH_ON"; break;
			case 7: eventName = "e_SWITCH_OFF"; break;
			case 8: eventName = "e_SWITCH_FAIL"; break;
			case 9: eventName = "e_FADE_START"; break;
			case 10: eventName = "e_FADE_MASTER"; break;
			case 11: eventName = "e_FAST_HEARTBEAT"; break;
			case 12: eventName = "e_NMEA_MESSAGE"; break;
			case 13: eventName = "e_NIGHTMODE"; break;
			case 14: eventName = "e_DAYLIGHTMODE"; break;
			case 15: eventName = "e_AMBIENT_LIGHT_LEVEL"; break;
			case 16: eventName = "e_BLACKOUT"; break;
			case 17: eventName = "e_SLOW_HEARTBEAT"; break;
			case 18: eventName = "e_THROTTLE_MASTER"; break;
                        case 19: eventName = "e_LED_LEVEL_CHANGED"; break;
			case 20: eventName = "e_CONFIG_FILE_UPDATE"; break;
			case 21: eventName = "e_SET_BACKLIGHT_LEVEL"; break;
                        case 22: eventName = "e_THROTTLE_CHANGE"; break;
                        default: eventName = "<unknown>";
			}

			line += buf.sprintf( "Dev %02d: %ls/%ls  Group %03d, Data %d, Info %d",
								 msg->data[ 4 ], funcName.utf16(),  eventName.utf16(),
								 msg->data[ 0 ], data,
								 msg->data[ 2 ] << 8 | msg->data[ 3 ] );
			break;
		}
	}

	ecsManagerApp::inst()->canusb_Instance->readCallback( line );
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
// Build NMEA PDU

void nmea_MakePDU( _u32 &outPGN,
	unsigned short pgn_priority,
	unsigned short pgn_no,
	unsigned short msg_bytes ) {

	outPGN =  _u32(pgn_no) << 8;
	outPGN |= _u32(pgn_priority) << 26;
}

void ecsCANUSB::sendTest( QByteArray &byteSequence ) {
	_u32 outPDU;
	CANMsg msg;
	int result;

	nmea_MakePDU( outPDU, 1, nmea_LIGHTING_COMMAND, 8 );

	msg.id = outPDU;
	msg.flags = CANMSG_EXTENDED;
	msg.timestamp = time( 0 );
	msg.len = 8;

	msg.data[0] = 0x11;
	msg.data[1] = 0x22;
	msg.data[2] = 0x33;
	msg.data[3] = 0x44;
	msg.data[4] = 0x55;
	msg.data[5] = 0x66;
	msg.data[6] = 0x77;
	msg.data[7] = 0x88;

	result = canusb_Write( adapterHandle, &msg );
}

void ecsCANUSB::sendConfig( QByteArray &configFile ) {

	_u32 outPDU;
	CANMsg msg;
	int msgLength;
	int noPackets;
	int sentBytes;
	int result;

	msgLength = configFile.length();
	noPackets = msgLength / 7;
	if( msgLength % 7 ) noPackets++;

	// First tell the bus we are about to transmit a multi-packet message.

	nmea_MakePDU( outPDU, 1, nmea_CM_BAM, 8 );

	msg.id = outPDU;
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

	nmea_MakePDU( outPDU, 1, nmea_DATATRANSFER, 8 );
	msg.id = outPDU;
	sentBytes = 0;

	for( int i=0; i<noPackets; i++ ) {

		msg.timestamp = time( 0 );
		msg.data[0] = i+1;

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
