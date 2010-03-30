/*
 * nmea.h
 *
 *  Created on: 28 mar 2010
 *      Author: Jesper W
 */

#ifndef NMEA_H_
#define NMEA_H_

// Parameter Group Names

#define nmea_DATATRANSFER			60160
#define nmea_CM_BAM				60416

#define nmea_LIGHTING_DATA			65088
#define nmea_LIGHTING_COMMAND		65089
#define nmea_MAINTAIN_POWER		65095

#define nmea_CONFIGURATION			126998

// NMEA2000 CA NAME basics for a marine electrical lighting appliance.

unsigned char 		nmea_CA_Address = 0x55;

#define nmea_STARTING_ADDRESS		128
#define nmea_ARBITRARY_ADDRESS		1	// Yes we can!
#define nmea_INDUSTRY_GROUP		4	// Marine = 4
#define nmea_VEHICLE_SYSTEM_INST	0

#define nmea_VEHICLE_SYSTEM		30	// Power Management and Lighting Systems

#define nmea_FUNCTION_SWITCH		130
#define nmea_FUNCTION_LOAD			140

#define nmea_ECU_INSTANCE			0

#define nmea_MANUFACTURER_CODE		2000		// 11 bits
#define nmea_IDENTITY_NUMBER		0x001700	// 21 bits

#define nmea_GLOBAL_ADDRESS		255
#define nmea_NULL_ADDRESS			254

#define nmea_NORMAL_MSG			0x0
#define nmea_REMOTE_TX_MSG			0x2
#define nmea_STANDARD_ID			0x0
#define nmea_EXTENDED_ID			0x1

unsigned char bytes;
unsigned char data[8];

#endif /* NMEA_H_ */
