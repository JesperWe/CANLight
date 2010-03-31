/*
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef ECSCANUSB_H
#define ECSCANUSB_H

#include "lawicel_can.h"
#include "Ftd2xx.h"

#include <QtGui>

class ecsCANUSB : public QObject
{
Q_OBJECT

public:

	enum canusb_e {
		DeviceError,
		DriverNotInstalled,
		DongleNotPresent,
		BusDisconnected,
		BusOpen,
		BusIdle,
		BusActive,
		ReceiveError,
		TransmitError
	};

	ecsCANUSB();

	int status();
	int open();
	int close();
	char* info();
	void registerReader();
	void unregisterReader();
	void sendConfig( QByteArray &configFile );
	void sendTest( QByteArray &byteSequence );
	void alert( QString location,  int result );
	QString errorString( int result );
	void readCallback( const QString &text );

private:
	int lastStatus;
	char adapterSerial[33];
	CANHANDLE adapterHandle;
	char adapterInfo[200];

signals:
	void addLogLine( const QString  & text );
};

#endif // ECSCANUSB_H
