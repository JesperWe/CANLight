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
unsigned char config_Invalid = 0;

//-------------------------------------------------------------------------------
// The system config represents the entire system, and is stored in each appliance.
// The config "file" is actually a byte sequence sent from some master controller
// to all appliances on update. This "file" is stored in User Flash memory and parsed at runtime.
//
// A "appliance" in the system is a single piece of hardware with its own CPU.
// It has an address (appliance id) in the range 0-253.
//
// A "function" is one individually controllable output on this appliance.
//
// A "group" is a collection of functions on one or many appliances that listen to the same events.
// A group can consist of only a single function on a single appliance.
// A function must be part of a group to be able to listen for events.
//
// The config "file" byte sequence follows this pattern:
//   <2 bytes magic number>
//   <2 bytes sequence number>
//   group appliance func [ appliance func ] FE
//   appliance func event [ event ] FE
//   appliance func event [ event ] FE

//  ^^^ change these to: event action appliance func [ appliance func ] FE
// (Better fit with GUI data model?)

//   FE
//   group appliance func [ appliance func ] FE
//   appliance func event [ event ] FE
//   appliance func event [ event ] FE
//   FE
//   ...
//   FF

// The Configuration File lives in program memory.
// The size limited to one Flash page, currently 1024 byte.


const unsigned char __attribute__((space(auto_psv),aligned(_FLASH_PAGE*2))) config_Data[_FLASH_PAGE*2];

//-------------------------------------------------------------------------------
// Utility functions to make config_Data navigation code more readable.

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

	configPtr = config_Data + 2;	// Skip sequence number.

	do {
		if( *configPtr == groupId ) return configPtr;
	} while (
		_findNextGroup( &configPtr )
	);

	return configPtr;
}


//-------------------------------------------------------------------------------
// Initialize System Configuration.
// The config_MyEvents dynamic list should contain all events that affect this device.

void config_Initialize() {
	config_File_t configPtr, targetPtr;
	unsigned short configSequenceNumber;
	unsigned char listenGroup;
	unsigned char func;

	// First check if we don't we have a valid device Id.
	// Device ID = 0xFF is not allowed. It indicates that the device was programmed
	// with default 0xFFFFFFFF in the Unit ID form of MPLAB.

	if( hw_DeviceID == 0xFF ) {
		config_Invalid = 4;
		return;
	}

	// Check for valid config file.

	if( (((unsigned short)config_Data[0])<<8 | ((unsigned short)config_Data[1])) != hw_CONFIG_MAGIC_WORD ) {
		config_Invalid = TRUE;
		return;
	}

	config_Invalid = FALSE;

	config_MyEvents = 0;

	configSequenceNumber = ((short)config_Data[2])<<8 | config_Data[3];

	// Now filter system config to find out what groups we belong to, and
	// which devices messages we should listen to.

	configPtr = config_Data+4;	// Skip magic and sequence numbers.

	do {
		listenGroup = *configPtr;
		targetPtr = configPtr + 1;

		do {
			if( *(targetPtr) == hw_DeviceID ) { // Is this group for me?
				func = *(targetPtr+1);
				config_AddMyControlEvents( listenGroup, configPtr, func );
			}
		} 
		while( _findNextTarget( &targetPtr ));
	}
	while( _findNextGroup( &configPtr ) );
}


void config_AddMyControlEvents( unsigned char listenGroup, config_File_t fromCfgPtr, unsigned char targetFunction ) {
	unsigned short cntrlEvent;
	unsigned short cntrlAction;
	unsigned short cntrlDevice;
	unsigned short cntrlFunction;

	_findControllers( &fromCfgPtr ); // Make fromCfgPtr point to start of the groups controllers.

	do {
		cntrlEvent = *fromCfgPtr++;
		cntrlAction = *fromCfgPtr++;
		
		while( *fromCfgPtr != config_GroupEnd ) {
			cntrlDevice = *fromCfgPtr++;
			cntrlFunction = *fromCfgPtr++;
			config_AddControlEvent( cntrlDevice, cntrlFunction, cntrlEvent, cntrlAction, targetFunction );
		}
		*fromCfgPtr++;
		
	} while( *fromCfgPtr != config_GroupEnd );

	return;
}


void config_AddControlEvent( 
		const unsigned char ctrlDev,
		const unsigned char ctrlFunc,
		const unsigned char ctrlEvent,
		const unsigned char ctrlAction,
		const unsigned char function )
{

	config_Event_t *newEvent;

	newEvent = malloc( sizeof( config_Event_t ) );
	newEvent->ctrlDev = ctrlDev;
	newEvent->ctrlFunc = ctrlFunc;
	newEvent->ctrlEvent = ctrlEvent;
	newEvent->ctrlAction = ctrlAction;
	newEvent->function = function;

	// Insert new event at start of eventlist.

	newEvent->next = config_MyEvents;
	config_MyEvents = newEvent;
}


//-------------------------------------------------------------------------------

void config_Task() {
	// No point in running if we have no valid system configuration,
	// so stay here waiting for one to arrive and flash something to show
	// we are waiting.

	unsigned long timer;
	short i;

	if( ! config_Invalid ) {
		schedule_AddTask( led_PowerOnTest, 100 );
		schedule_Finished();
		return;
	}

	while ( config_Invalid ) {

		// We hijack the scheduler in this loop.
		// We will hang here until the NMEA Interrupt Service Routine has received
		// a valid system configuration over the network.

		schedule_Running = FALSE;

		_TRISB5 = 0;
		_TRISB11 = 0;

		timer = schedule_time + 800;
		while( schedule_time < timer );

		// Number of flashes indicates type of config problem.

		for( i=0; i< config_Invalid; i++ ) {
			_RB5 = 1;
			_RB11 = 1;
			led_SetLevel( led_RED, 1.0);

			timer = schedule_time + 200;
			while( schedule_time < timer );

			_RB5 = 0;
			_RB11 = 0;
			led_SetLevel( led_RED, 0.0);

			timer = schedule_time + 200;
			while( schedule_time < timer );

		}
	}


	schedule_Running = TRUE;
	schedule_AddTask( led_TaskComplete, 100 );
	schedule_Finished();

}

//-------------------------------------------------------------------------------
// We have received a new system configuration over the NMEA bus.
// Store it in flash and start using it.

void config_Update( unsigned short configBytes ) {
	int i;
	int *newConfigData;
	_prog_addressT config_FlashPage;

	config_Invalid = TRUE;

	// Save the config here...

	_init_prog_address( config_FlashPage, config_Data );

	_erase_flash( config_FlashPage );

	newConfigData = (int*)nmea_LargeBuffer;

	for( i = 0; i < (_FLASH_PAGE / _FLASH_ROW); i++ ) {
		_write_flash16( config_FlashPage, newConfigData );
		config_FlashPage += _FLASH_ROW * 2;
		newConfigData += _FLASH_ROW * 2;
	}

	config_Invalid = FALSE;

}
