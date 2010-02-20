/*
 * Revision $Rev$
 * By $Author$
 * Date $Date$
 */

#ifndef ECSCANUSB_H
#define ECSCANUSB_H

#include "lawicel_can.h"
#include "Ftd2xx.h"

class ecsCANUSB
{
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

private:
	int lastStatus;
	char adapterSerial[33];
	CANHANDLE adapterHandle;
	char adapterInfo[200];

};

#endif // ECSCANUSB_H
