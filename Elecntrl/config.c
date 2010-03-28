#include <stdlib.h>
#include <libpic30.h>

#include "hw.h"
#include "schedule.h"
#include "config.h"
#include "events.h"
#include "led.h"
#include "display.h"
#include "menu.h"
#include "nmea.h"

//-------------------------------------------------------------------------------
// Globals

config_Event_t *config_MyEvents;
unsigned char cfg_MyDeviceId = 0;
unsigned char config_Valid = 0;

//-------------------------------------------------------------------------------
// The system config represents the entire system, and is stored in each device.
// The config "file" is actually a byte sequence sent from some master controller
// to all devices on update. This "file" is stored in User Flash memory and parsed at runtime.
//
// A "device" in the system is a single piece of hardware with its own CPU.
// It has an address (device number) in the range 0-253.
//
// A "function" is one individually controllable output on this device.
//
// A "group" is a collection of functions on one or many devices that listen to the same events.
// A group can consist of only a single function on a single device.
// A function must be part of a group to be able to listen for events.
//
// The config "file" byte sequence follows this pattern:
//   <2 bytes sequence number>
//   group device func [ device func ] FE
//   device func event [ event ] FE
//   device func event [ event ] FE

//  ^^^ change these to: event action device func [ device func ] FE
// (Better fit with GUI data model?)

//   FE
//   group device func [ device func ] FE
//   device func event [ event ] FE
//   device func event [ event ] FE
//   FE
//   ...
//   FF

// The Configuration File lives in program memory.
// The size is dictated by the maximum length of a NMEA transmission,

//static const unsigned char __attribute__((space(prog),aligned(_FLASH_PAGE*2))) config_File[1700];
static const unsigned char config_File[1785];

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

	// Check for valid config file.

	if( (((unsigned short)config_File[0])<<8 | ((unsigned short)config_File[1])) != hw_CONFIG_MAGIC_WORD ) {
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

	unsigned long timer;

	while ( ! config_Valid ) {

		schedule_Running = FALSE;  // We hijack the scheduler in this loop.

		_TRISB5 = 0;
		_TRISB11 = 0;

		timer = schedule_time + 1000;
		while( schedule_time < timer );

		_RB5 = 1;
		_RB11 = 1;
		led_SetLevel( led_RED, 1.0);

		timer = schedule_time + 400;
		while( schedule_time < timer );

		_RB5 = 0;
		_RB11 = 0;
		led_SetLevel( led_RED, 0.0);
	}


	schedule_Running = TRUE;

	short status;

	schedule_AddTask( led_PowerOnTest, 100 );
	if( status != 0 ) {
		status = 0;
	}
	schedule_Finished();

}

//-------------------------------------------------------------------------------
// We have received a new system configuration over the NMEA bus.
// Store it in flash and start using it.

void config_Update( unsigned short configBytes ) {

	config_Valid = FALSE;

	// XXX Save the config here...

	config_Valid = TRUE;

}
