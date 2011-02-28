#include <stdlib.h>
#include <libpic30.h>

#include "hw.h"
#include "schedule.h"
#include "events.h"
#include "led.h"
#include "display.h"
#include "menu.h"
#include "nmea.h"
#include "config.h"

/*------------------------------------------------------------------------------------

The system config represents the entire yacht network, and is stored in each appliance.
The config "file" is actually a byte sequence sent on update from some master controller
to all appliances. This "file" is stored in User Flash memory and parsed at runtime.

A "appliance" in the system is a single piece of hardware with its own CPU.
It has an address (Appliance ID) in the range 0-253.

A "function" is one individually controllable I/O channel on this appliance. A function can also
bundle several channels, like the "Lamp" function is a bundle of all colors of a lamp appliance.

A "group" is a collection of functions on one or many appliances that listen to the same events.
Group ID is also in the 0-253 range.
A group can consist of only a single function of a single appliance.
A function must be part of a group to be able to listen for events.

The config "file" byte sequence follows this pattern:


The events sent between the controllers and the listeners are always send as broadcast messages,
but each message uses the group IDs to mark which groups are communicating.

Communication flow:

Controller Group (GID)
        Appliance (AID)/Function

>>> Sends: Controller Group ID + Event

Bindings: [Event/Action], ...

<<< Sends: Listener Group ID + Event

Listener Group (GID)
        Appliance (AID)/Function

*/


//-------------------------------------------------------------------------------
// Globals

unsigned char cfg_MyDeviceId = 0;
unsigned char config_Invalid = 0;
unsigned char config_CurrentGroup;

//-------------------------------------------------------------------------------


// The Configuration File lives in program memory.
// The size limited to one Flash page, currently 1024 byte.

const unsigned char __attribute__((space(auto_psv),aligned(_FLASH_PAGE*2))) config_Data[_FLASH_PAGE*2];


//-------------------------------------------------------------------------------
// Find the first group that a port on this device is a member of.

unsigned char config_GetGroupIdForPort( unsigned char port ) {
	unsigned char controllerGroupID;
	unsigned char confDevice;
	unsigned char confPort;

	const unsigned char* configPtr = config_Data;
	configPtr += 4;	// Skip magic and sequence numbers.

	do {
		controllerGroupID = *configPtr++;

		do {
			confDevice = *configPtr++;
			confPort = *configPtr++;

			if( confDevice == hw_DeviceID && confPort == port ) {
				return controllerGroupID;
			}
		}
		while( *configPtr != DELIMITER );

		// Roll passed listener groups.

		configPtr++;
		do { configPtr++; }
		while( *configPtr != DELIMITER );

		// Roll passed event/actions.

		configPtr++;
		do { configPtr++; }
		while( *configPtr != DELIMITER );

		while( *configPtr == DELIMITER ) { configPtr++; }
	}
	while( *configPtr != END_OF_FILE );

	return config_GROUP_UNDEFINED;
}


//-------------------------------------------------------------------------------
// Return the action to take if we are listening to this event, or NO_ACTION if we are not.

unsigned char config_GetPortActionFromEvent( unsigned char port, event_t* event ) {
	unsigned char controllerGroupID;
	unsigned char confDevice;
	unsigned char confPort;
	unsigned char listenerGroupID;
	unsigned char functionIsInCurrentTask;
	unsigned char eventType;
	unsigned char action;
	unsigned char defaultAction;
	unsigned char eventIsInCurrentGroup;
	unsigned char sendingGroupIsTask;
	unsigned char portIsInCurrentCntrlGroup;


	const unsigned char* configPtr = config_Data;
	configPtr += 4;	// Skip magic and sequence numbers.

	eventIsInCurrentGroup = FALSE;
	config_CurrentGroup = 0;

	do {
		defaultAction = a_NO_ACTION;
		functionIsInCurrentTask = FALSE;
		portIsInCurrentCntrlGroup = FALSE;

		// Controller Group. Is this the group that generated the event?

		controllerGroupID = *configPtr++;

		if ( controllerGroupID == event->groupId ) {
			eventIsInCurrentGroup = TRUE;
			config_CurrentGroup = controllerGroupID;
		}

		do {
			confDevice = *configPtr++;
			confPort = *configPtr++;

			// For acknowledge events, we need to remember if this control group
			// contains the current device and port.

			if( confDevice == hw_DeviceID && confPort == port )
				portIsInCurrentCntrlGroup = TRUE;

		}
		while( *configPtr != DELIMITER );

		// Listener Group Devices. See if we are a listener?

		configPtr++;
		listenerGroupID = *configPtr++;

		sendingGroupIsTask = ( listenerGroupID == event->groupId );

		do {
			confDevice = *configPtr++;
			confPort = *configPtr++;

			if( eventIsInCurrentGroup ) {

				if(	( confDevice == hw_DeviceID || confDevice == hw_DEVICE_ANY ) &&
					( port == confPort ) )
				{
					functionIsInCurrentTask = TRUE;
					continue;
				}
			}

			// Fading events: If the current port is in the same task group as the
			// Fade Master (but not the fade master itself!) then we map some events
			// to the corresponding actions.

			if(	sendingGroupIsTask && portIsInCurrentCntrlGroup )
			{
				if(	event->ctrlEvent == e_LED_LEVEL_CHANGED ) { defaultAction = a_SET_LEVEL; }
				if(	event->ctrlEvent == e_FADE_START ) { defaultAction = a_FADE_MASTER_ARBITRATION; }
			}
		}
		while( *configPtr != DELIMITER );

		// OK, so what action does this event generate for this port?

		configPtr++;
		do {
			eventType = *configPtr++;
			action = *configPtr++;

			if( ! eventIsInCurrentGroup ) continue;
			if( ! functionIsInCurrentTask ) continue;

			if( eventType == event->ctrlEvent ) {

				// OK we have found the action to perform for this event. Since multiple actions
				// for the same event/port combination are not supported we can stop searching
				// and return immediately.

				return action;
			}
		}
		while( *configPtr != DELIMITER );

		while( *configPtr == DELIMITER ) { configPtr++; }
	}
	while( *configPtr != END_OF_FILE );

	return defaultAction;
}


//-------------------------------------------------------------------------------
// Initialize System Configuration.
// The config_MyEvents dynamic list should contain all events that affect this device.

void config_Initialize() {
	unsigned short configSequenceNumber;

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
	configSequenceNumber = ((short)config_Data[2])<<8 | config_Data[3];
}



//-------------------------------------------------------------------------------

void config_UninitializedTask() {
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

		if( hw_Type == hw_LEDLAMP ) led_SetLevel( led_WHITE, 0.0, led_NO_ACK );

		timer = schedule_time + schedule_SECOND;
		while( schedule_time < timer );

		// Number of flashes indicates type of config problem.

		for( i=0; i< config_Invalid; i++ ) {
			led_SetLevel( led_RED, 1.0, led_NO_ACK );

			timer = schedule_time + schedule_SECOND/5;
			while( schedule_time < timer );

			led_SetLevel( led_RED, 0.0, led_NO_ACK);

			timer = schedule_time + schedule_SECOND/5;
			while( schedule_time < timer );

			asm volatile ("CLRWDT");
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

	if( hw_1kBuffer[0] != 0x12 ||
		hw_1kBuffer[1] != 0x69 ) return;

	// Don't save the new config if it is the same we already have in flash.

	if( hw_1kBuffer[2] == config_Data[2] &&
		hw_1kBuffer[3] == config_Data[3] ) return;

	// Reality check on the length.

	if( configBytes < 10 || configBytes > 1024 ) return;

	// Save the config.

	_init_prog_address( config_FlashPage, config_Data );

	_erase_flash( config_FlashPage );

	newConfigData = (int*)hw_1kBuffer;

	for( i = 0; i < (_FLASH_PAGE / _FLASH_ROW); i++ ) {
		_write_flash16( config_FlashPage, newConfigData );
		config_FlashPage += _FLASH_ROW * 2;
		newConfigData += _FLASH_ROW;
	}

	config_Invalid = FALSE;
	config_Initialize();
	schedule_Parameter = 4;
	schedule_AddTask( led_TaskComplete, schedule_SECOND/10 );

}
