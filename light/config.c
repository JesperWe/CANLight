#include <stdlib.h>
#include <libpic30.h>

#include "hw.h"
#include "schedule.h"
#include "config.h"
#include "config_groups.h"
#include "events.h"
#include "led.h"

//-------------------------------------------------------------------------------
// Globals

config_Event_t *config_MyEvents;
unsigned char cfg_MyDeviceId = 0;
unsigned char config_Valid = 0;

#include "config_groups.h"

static const unsigned char cfg_FileROM[1785] = cfg_DEFAULT_CONFIG_FILE;

//-------------------------------------------------------------------------------
// System Configuration.

void config_Initialize() {
	unsigned char cfgByte;
	unsigned char *cfgPtr;
	unsigned short i;
	unsigned short cfgSequenceNumber;
	unsigned char group;
	unsigned char func;

	// First check if we don't we have a valid device Id.
	// Device ID = 0xFF is not allowed. It indicates that the device was programmed
	// with default 0xFFFFFFFF in the Unit ID form of MPLAB.

	if( hw_DeviceID == 0xFF ) {
		config_Valid = 0;
		return;
	}

	// Allocate first empty element for my controllers list.

	config_MyEvents = malloc( sizeof( config_Event_t ) ); 
	config_MyEvents->group = gEnd; // Indicates empty entry.
	config_MyEvents->next = 0;

	cfgSequenceNumber = ((short)cfg_FileROM[0])<<8 | cfg_FileROM[1];

	// Now filter system config to find out what groups we belong to, and
	// which devices messages we should listen to.

	cfgPtr = cfg_FileROM + 2;	// Skip sequence number.

	while( *cfgPtr != endConfig ) {
		group = *cfgPtr;
		cfgPtr++; // Go to first device of current group.

		while( *cfgPtr != gEnd ) {
			if( *cfgPtr == hw_DeviceID ) {
				cfgPtr++;
				func = *cfgPtr;
				config_AddControlEvents( group, cfgPtr, func );
			}
			cfgPtr++;
		}

		// Now skip to start of next group.

		while(*cfgPtr!=gEnd || *(cfgPtr-1)!=gEnd) { cfgPtr++; };
		cfgPtr++;
	}
}


void config_AddControlEvents( unsigned char group, unsigned char *fromCfgPtr, unsigned char func ) {
	unsigned short i;

	while(1) {
		// First find the controllers start
		
		while( *fromCfgPtr != gEnd ) fromCfgPtr++;
		fromCfgPtr++;
		if( *fromCfgPtr == gEnd ) break; // Two gEnd in a row is end of group.
		if( *fromCfgPtr == endConfig ) break;
		
		i=2;
		while( *(fromCfgPtr+i) != gEnd ) {
			config_AddControlEvent( group, *fromCfgPtr, *(fromCfgPtr+1), *(fromCfgPtr+i), func );
			i++;
		}
	}
	return;
}


void config_AddControlEvent( unsigned char group, 
		unsigned char ctrlDev, 
		unsigned char ctrlFunc, 
		unsigned char ctrlEvent, 
		unsigned char function )
{

	config_Event_t *curEvent = config_MyEvents;

	config_Event_t *newEvent;

	while( curEvent->next != 0 ) curEvent = curEvent->next;

	newEvent = curEvent;

	if( newEvent->group != gEnd ) {
		newEvent = malloc( sizeof( config_Event_t ) );
		newEvent->next = 0;
		curEvent->next = newEvent;
	}

	newEvent->group = group;
	newEvent->ctrlDev = ctrlDev;
	newEvent->ctrlFunc = ctrlFunc;
	newEvent->ctrlEvent = ctrlEvent;
	newEvent->function = function;
}


void config_Task() {
	// No point in running if we have no valid system configuration,
	// so stay here waiting for one to arrive and flash something to show
	// we are waiting.

	if ( ! config_Valid ) {

		_TRISB5 = 0;
		_TRISB11 = 0;

		schedule_Sleep(1000);

		_RB5 = 1;
		_RB11 = 1;

		schedule_Sleep(400);

		_RB5 = 0;
		_RB11 = 0;
	}

	else {
		short status;
		schedule_AddTask( led_PowerOnTest, 100 );
		if( status != 0 ) {
			status = 0;
		}
		schedule_Finished();
	}
}

