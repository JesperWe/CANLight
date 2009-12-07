#include <stdlib.h>
#include <libpic30.h>

#include "hw.h"
#include "events.h"
#include "config.h"
#include "config_groups.h"

//-------------------------------------------------------------------------------
// Globals

cfg_Event_t *cfg_MyEvents;
unsigned char cfg_MyDeviceId = 0;
unsigned char config_Valid = 0;

#include "config_groups.h"

static const unsigned char cfg_Default[] = cfg_DEFAULT_CONFIG_FILE;

//-------------------------------------------------------------------------------
// System Configuration.

void config_Initialize() {
	unsigned char cfgByte;
	unsigned char *cfgPtr;
	unsigned short i; 
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

	cfg_MyEvents = malloc( sizeof( cfg_Event_t ) ); 
	cfg_MyEvents->group = gEnd; // Indicates empty entry.
	cfg_MyEvents->next = 0;

	// Copy default config to RAM config if there is no config in Flash.

	if( hw_Config.cfgSequenceNumber == 0 ) {
		i = 0;
		cfgByte = cfg_Default[i];
		while( cfgByte != 0xFF ) {
			hw_Config.cfgFile[i] = cfgByte;
			cfgByte = cfg_Default[++i];
		}
		hw_Config.cfgFile[i] = cfgByte;
		if( i > 0 ) config_Valid = 1;
	}

	// Now filter system config to find out what groups we belong to, and
	// which devices messages we should listen to.

	cfgPtr = hw_Config.cfgFile;

	while( *cfgPtr != endConfig ) {
		group = *cfgPtr;
		cfgPtr++; // Go to first device of current group.

		while( *cfgPtr != gEnd ) {
			if( *cfgPtr == hw_DeviceID ) {
				cfgPtr++;
				func = *cfgPtr;
				cfg_AddControlEvents( group, cfgPtr, func );
			}
			cfgPtr++;
		}

		// Now skip to start of next group.

		while(*cfgPtr!=gEnd || *(cfgPtr-1)!=gEnd) { cfgPtr++; };
		cfgPtr++;
	}
}


void cfg_AddControlEvents( unsigned char group, unsigned char *fromCfgPtr, unsigned char func ) {
	unsigned short i;

	while(1) {
		// First find the controllers start
		
		while( *fromCfgPtr != gEnd ) fromCfgPtr++;
		fromCfgPtr++;
		if( *fromCfgPtr == gEnd ) break; // Two gEnd in a row is end of group.
		if( *fromCfgPtr == endConfig ) break;
		
		i=2;
		while( *(fromCfgPtr+i) != gEnd ) {
			cfg_AddControlEvent( group, *fromCfgPtr, *(fromCfgPtr+1), *(fromCfgPtr+i), func );
			i++;
		}
	}
	return;
}


void cfg_AddControlEvent( unsigned char group, 
		unsigned char ctrlDev, 
		unsigned char ctrlFunc, 
		unsigned char ctrlEvent, 
		unsigned char function )
{

	cfg_Event_t *curEvent = cfg_MyEvents;

	cfg_Event_t *newEvent;

	while( curEvent->next != 0 ) curEvent = curEvent->next;

	newEvent = curEvent;

	if( newEvent->group != gEnd ) {
		newEvent = malloc( sizeof( cfg_Event_t ) );
		newEvent->next = 0;
		curEvent->next = newEvent;
	}

	newEvent->group = group;
	newEvent->ctrlDev = ctrlDev;
	newEvent->ctrlFunc = ctrlFunc;
	newEvent->ctrlEvent = ctrlEvent;
	newEvent->function = function;
}
