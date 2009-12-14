#include <stdlib.h>
#include <libpic30.h>

#include "hw.h"
#include "schedule.h"
#include "config.h"
#include "config_groups.h"
#include "events.h"
#include "led.h"
#include "display.h"
#include "menu.h"

//-------------------------------------------------------------------------------
// Globals

config_Event_t *config_MyEvents;
unsigned char cfg_MyDeviceId = 0;
unsigned char config_Valid = 0;

#include "config_groups.h"

// The Configuration File lives in program memory.
// The size is dictated by the maximum length of a NMEA transmission,
// which is 7*255 = 1785 bytes.

static const unsigned char config_File[1785] = cfg_DEFAULT_CONFIG_FILE;

//-------------------------------------------------------------------------------
// Utility functions to make navigation of the config_File easier to understand.

unsigned char _findNextGroup( config_File_t* filePointer ) {
	config_File_t tmpPointer = *filePointer;

	while( (*tmpPointer != config_GroupEnd) || (*(tmpPointer+1) != config_GroupEnd) ) {
		tmpPointer++;
	}

	tmpPointer += 2;
	*filePointer = tmpPointer;

	if( **filePointer == endConfig ) return FALSE;

	return TRUE;
}

unsigned char _findNextTarget( config_File_t* filePointer ) {
	(*filePointer) += 2;
	if( **filePointer == config_GroupEnd ) return FALSE;
	return TRUE;
}

unsigned char _findControllers( config_File_t* filePointer ) {
	while( (**filePointer != config_GroupEnd) ) (*filePointer)++;
	(*filePointer)++;
	return TRUE;
}

config_File_t config_FileFindGroup( unsigned char groupId ) {
	config_File_t configPtr;

	configPtr = config_File + 2;	// Skip sequence number.

	do {
		if( *configPtr == groupId ) return configPtr;
	} while (
		_findNextGroup( &configPtr )
	);

	return configPtr;
}

short config_FileCountTargets( unsigned char groupId ) {
	config_File_t configPtr;
	unsigned char noTargets;

	configPtr = config_File + 2;	// Skip sequence number.
	return noTargets;
}


//-------------------------------------------------------------------------------
// Initialize System Configuration.
// The config_MyEvents dynamic list should contain all events that affect this device.

void config_Initialize() {
	config_File_t configPtr, targetPtr;
	unsigned short configSequenceNumber;
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
	config_MyEvents->group = config_GroupEnd; // Indicates empty entry.
	config_MyEvents->next = 0;

	configSequenceNumber = ((short)config_File[0])<<8 | config_File[1];

	// Now filter system config to find out what groups we belong to, and
	// which devices messages we should listen to.

	configPtr = &(config_File[2]);	// Skip sequence number.

	do {
		group = *configPtr;
		targetPtr = configPtr + 1;

		do {
			if( *(targetPtr) == hw_DeviceID ) { // Is this group for me?
				func = *(targetPtr+1);
				config_AddMyControlEvents( group, configPtr, func );
			}
		} 
		while( _findNextTarget( &targetPtr ));
	}
	while( _findNextGroup( &configPtr ) );
}


void config_AddMyControlEvents( unsigned char group, config_File_t fromCfgPtr, unsigned char targetFunction ) {
	unsigned short cntrlDevice, cntrlFunction;

	_findControllers( &fromCfgPtr ); // Note that fromCfgPointer is call-by-value so this will not modify callers pointer!

	do {
		cntrlDevice = *fromCfgPtr++;
		cntrlFunction = *fromCfgPtr++;
		
		while( *fromCfgPtr != config_GroupEnd ) {
			config_AddControlEvent( group, cntrlDevice, cntrlFunction, *fromCfgPtr++, targetFunction );
		}
		*fromCfgPtr++;
		
	} while( *fromCfgPtr != config_GroupEnd );

	return;
}


void config_AddControlEvent( unsigned char group, 
		const unsigned char ctrlDev,
		const unsigned char ctrlFunc,
		const unsigned char ctrlEvent,
		const unsigned char function )
{

	config_Event_t *curEvent = config_MyEvents;

	config_Event_t *newEvent;

	while( curEvent->next != 0 ) curEvent = curEvent->next;

	newEvent = curEvent;

	newEvent = malloc( sizeof( config_Event_t ) );
	newEvent->next = 0;
	curEvent->next = newEvent;

	newEvent->group = group;
	newEvent->ctrlDev = ctrlDev;
	newEvent->ctrlFunc = ctrlFunc;
	newEvent->ctrlEvent = ctrlEvent;
	newEvent->function = function;
}


//-------------------------------------------------------------------------------

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

