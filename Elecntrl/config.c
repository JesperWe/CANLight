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

/*------------------------------------------------------------------------------------

The system config represents the entire system, and is stored in each appliance.
The config "file" is actually a byte sequence sent from some master controller
to all appliances on update. This "file" is stored in User Flash memory and parsed at runtime.

A "appliance" in the system is a single piece of hardware with its own CPU.
It has an address (Appliance ID) in the range 0-253.

A "function" is one individually controllable I/O channel on this appliance. A function can also
bundle several channels, like the "Lamp" function is a bundle of all colors of a lamp appliance.

Controller Group (GID)
        Appliance (AID)/Function

>>> Sends: Controller Group ID + Event

Bindings: [Event/Action], ...

<<< Sends: Listener Group ID + Event

Listener Group (GID)
        Appliance (AID)/Function


A "group" is a collection of functions on one or many appliances that listen to the same events.
Group ID is also in the 0-253 range.
A group can consist of only a single function of a single appliance.
A function must be part of a group to be able to listen for events.

The config "file" byte sequence follows this pattern:
	<2 bytes magic number>
	<2 bytes sequence number>
	ControllerGroupID ApplianceID func [ ApplianceID func ] FE
	ListenerGroupID ApplianceID func [ ApplianceID func ] FE
	event action [event action] FE
	FE
	...
	FF

The events sent between the controllers and the listeners are always send as broadcast messages,
but each message uses the group IDs to mark which groups are communicating.
*/


//-------------------------------------------------------------------------------
// Globals

unsigned char cfg_MyDeviceId = 0;
unsigned char config_Invalid = 0;
unsigned char functionInGroup[ hw_NoFunctions ];
unsigned char functionListenGroup[ hw_NoFunctions ];
config_Event_t *config_MyEvents;


//-------------------------------------------------------------------------------


// The Configuration File lives in program memory.
// The size limited to one Flash page, currently 1024 byte.

const unsigned char __attribute__((space(auto_psv),aligned(_FLASH_PAGE*2))) config_Data[_FLASH_PAGE*2];

//-------------------------------------------------------------------------------
// Initialize System Configuration.
// The config_MyEvents dynamic list should contain all events that affect this device.

void config_Initialize() {
	unsigned char* configPtr;
	unsigned short configSequenceNumber;
	unsigned char inGroupID;
	unsigned char listenToGroupID;
	unsigned char applianceID;
	unsigned char functionID;
	unsigned char event;
	unsigned char action;

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

	config_MyEvents = 0; // XXX Memory leak if config initialized more than once after reset!

	for( functionID=0; functionID<hw_NoFunctions; functionID++ ) {
		functionListenGroup[ functionID ] = 0;
		functionInGroup[ functionID ] = 0;
	}

	configSequenceNumber = ((short)config_Data[2])<<8 | config_Data[3];

	// Now filter system config to find out what groups we belong to, and
	// which devices messages we should listen to.
	
	configPtr = config_Data;
	configPtr += 4;	// Skip magic and sequence numbers.

	do {

		// Controller Group Appliances

		inGroupID = *configPtr++;
		do {
			applianceID = *configPtr++;
			functionID = *configPtr++;

			if( applianceID == hw_DeviceID ) {
				functionInGroup[ functionID ] = inGroupID;
			}
		}
		while( *configPtr != DELIMITER );

		// Listener Group Appliances. These functions are "in" the listenToGroupID,
		// but "listen" to the inGroupID.

		configPtr++;
		listenToGroupID = *configPtr++;
		do {
			applianceID = *configPtr++;
			functionID = *configPtr++;

			if( applianceID == hw_DeviceID ) {
				functionInGroup[ functionID ] = listenToGroupID;
				functionListenGroup[ functionID ] = inGroupID;
			}
		} 
		while( *configPtr != DELIMITER );

		// Since the controlling group came first, we need to go through the functions
		// in it and set their listening group for acknowledge events.

		for( functionID=0; functionID<hw_NoFunctions; functionID++ ) {
			if( functionInGroup[ functionID ] == inGroupID ) {
				functionListenGroup[ functionID ] = listenToGroupID;
			}
		}

		// Event / Action bindings

		configPtr++;
		do {
			event = *configPtr++;
			action = *configPtr++;
			config_AddControlEvent( inGroupID, event, action );
		}
		while( *configPtr != DELIMITER );

		while( *configPtr == DELIMITER ) { configPtr++; }
	}
	while( *configPtr != END_OF_FILE );
}


void config_AddControlEvent( 
		const unsigned char ctrlGroup,
		const unsigned char ctrlEvent,
		const unsigned char ctrlAction )
{

	config_Event_t *newEvent;

	newEvent = malloc( sizeof( config_Event_t ) );
	newEvent->ctrlGroup = ctrlGroup;
	newEvent->ctrlEvent = ctrlEvent;
	newEvent->ctrlAction = ctrlAction;

	// Insert new event at start of event list.

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
		schedule_AddTask( led_PowerOnTest, schedule_SECOND/10 );
		schedule_Finished();
		return;
	}

	while ( config_Invalid ) {

		// We hijack the scheduler in this loop.
		// We will hang here until the NMEA Interrupt Service Routine has received
		// a valid system configuration over the network.

		schedule_Running = FALSE;

		if( hw_Type == hw_LEDLAMP ) led_SetLevel( led_WHITE, 0.0 );

		timer = schedule_time + schedule_SECOND;
		while( schedule_time < timer );

		// Number of flashes indicates type of config problem.

		for( i=0; i< config_Invalid; i++ ) {
			led_SetLevel( led_RED, 1.0);

			timer = schedule_time + schedule_SECOND/5;
			while( schedule_time < timer );

			led_SetLevel( led_RED, 0.0);

			timer = schedule_time + schedule_SECOND/5;
			while( schedule_time < timer );
		}
	}

	schedule_Running = TRUE;
	schedule_Finished();

}

//-------------------------------------------------------------------------------
// We have received a new system configuration over the NMEA bus.
// Store it in flash and start using it.

void config_Update( unsigned short configBytes ) {
	unsigned short i;
	int *newConfigData;
	_prog_addressT config_FlashPage;

	// Does it have a magic number?

	if( nmea_LargeBuffer[0] != 0x12 ||
		nmea_LargeBuffer[1] != 0x69 ) return;

	// Don't save the new config if it is the same we already have in flash.

	if( nmea_LargeBuffer[2] == config_Data[2] &&
		nmea_LargeBuffer[3] == config_Data[3] ) return;

	// Reality check on the length.

	if( configBytes < 10 || configBytes > 1024 ) return;

	// Save the config.

	_init_prog_address( config_FlashPage, config_Data );

	_erase_flash( config_FlashPage );

	newConfigData = (int*)nmea_LargeBuffer;

	for( i = 0; i < (_FLASH_PAGE / _FLASH_ROW); i++ ) {
		_write_flash16( config_FlashPage, newConfigData );
		config_FlashPage += _FLASH_ROW * 2;
		newConfigData += _FLASH_ROW * 2;
	}

	config_Invalid = FALSE;
	config_Initialize();
	schedule_AddTask( led_TaskComplete, schedule_SECOND/10 );

}
