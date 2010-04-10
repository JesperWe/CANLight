/*
 * nmea.h
 *
 *  Created on: 2009-jun-01
 *      Author: Jesper W
 */

#ifndef NMEA_H_
#define NMEA_H_

#define __debug_pulse(pin)		pin = 1; NOP; pin = 0; NOP;

#define nmea_INTERFACE_ENABLED		0
#define nmea_INTERFACE_DISABLED	1

// Return Codes.

#define nmea_SUCCESS				0
#define nmea_QUEUEEMPTY			1
#define nmea_QUEUEFULL				1
#define nmea_CANNOTRECEIVE			2
#define nmea_CANNOTTRANSMIT		2
#define nmea_PARAMERROR			3
#define nmea_TRANSMITTER_BUSY		4


// Parameter Group Names

#define nmea_DATATRANSFER			60160
#define nmea_CM_BAM				60416

#define nmea_LIGHTING_DATA			65088
#define nmea_LIGHTING_COMMAND		65089
#define nmea_MAINTAIN_POWER		65095

#define nmea_CONFIGURATION			126998

// NMEA2000 CA NAME basics for a marine electrical lighting appliance.

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

// Message and Message Buffer sizes.

#define nmea_MSG_HEADER_LENGTH		5
#define nmea_MSG_BUFFER_WORDS		8
#define nmea_MSG_BUFFER_BYTES		2*nmea_MSG_BUFFER_WORDS
#define nmea_MAX_TP_PACKETS		255
#define nmea_TP_PACKET_BYTES		7

#define nmea_NO_MSG_BUFFERS		4
#define nmea_MSG_DATA_BYTES		8

#define nmea_RX_QUEUE_SIZE			3
#define nmea_TX_QUEUE_SIZE			3

#define nmea_TX_REQUEST_BIT		C1TR01CONbits.TXREQ0

typedef unsigned short nmea_MsgBuffer_t[nmea_MSG_BUFFER_WORDS];

// NB!! Reversed byte order due to little endian storage.

union nmea_PDU_UNION {
	struct {
		unsigned char SourceAddress;
		unsigned char PDUSpecific;
		unsigned char PDUFormat;
		unsigned Datapage			: 1;
		unsigned _Reserved			: 1;
		unsigned Priority			: 3;
		unsigned char bytes;
		unsigned char data[8];
		unsigned short PGN;
	};
	unsigned long PDU;
};

typedef union nmea_PDU_UNION nmea_PDU_t;


union nmea_FLAGS_UNION {
	struct {
		unsigned CannotClaimAddress					: 1;
		unsigned WaitingForAddressClaimContention	: 1;
		unsigned GettingCommandedAddress			: 1;
		unsigned GotFirstDataPacket					: 1;
		unsigned ReceivedMessagesDropped			: 1;
	};
	unsigned char all;
};

typedef union nmea_FLAGS_UNION nmea_FLAG;


typedef	struct {
	unsigned vehicle_syst_inst	: 4;
	unsigned industry_group		: 3;
	unsigned arbitrary_addr		: 1;

	unsigned reserved			: 1;
	unsigned vehicle_syst		: 7;

	unsigned char function;

	unsigned ecu_inst			: 3;
	unsigned function_inst		: 5;

	unsigned long identity		: 21;
	unsigned manufacturer		: 11;
} nmea_CA_NAME_t;




//---------------------------------------------------------------------------------------------
// Globals.

extern unsigned char 	nmea_CA_Address;
extern nmea_FLAG    	nmea_Flags;
extern unsigned char	RXQueueCount;
extern nmea_MsgBuffer_t	nmea_MsgBuf[nmea_NO_MSG_BUFFERS] __attribute__((space(dma)));
extern nmea_CA_NAME_t 	nmea_CA_Name;
extern nmea_MsgBuffer_t	inMessage;
extern nmea_PDU_t 		inPDU, nmea_OutgoingPDU;
extern nmea_MsgBuffer_t	nmea_TxQueue[nmea_NO_MSG_BUFFERS];
extern unsigned char	nmea_TxQueueHead;
extern unsigned char	nmea_TxQueueTail;
extern unsigned char	nmea_TxQueueFull;
extern WORD				nmea_HW_EN_Reg_Addr;
extern WORD				nmea_HW_EN_Reg_Bit;
extern WORD				nmea_HW_Rate_Reg_Addr;
extern WORD				nmea_HW_Rate_Reg_Bit;

extern unsigned char		nmea_TPMessage_Complete;
extern short 				nmea_TPMessage_Size;
extern long				nmea_TPMessage_PGN;


//---------------------------------------------------------------------------------------------
// Prototypes.

void nmea_Initialize();

void nmea_Wakeup();

void nmea_MakePGN( 
		unsigned short pdn_priority,
		unsigned short pgn_no,
		unsigned short msg_bytes );

unsigned char nmea_SendEvent( event_t *event );

unsigned char nmea_SendKeyEvent( event_t *event );

unsigned char nmea_SendMessage();

int nmea_DistributeSettings();

void nmea_ControllerMode( unsigned char mode );

void nmea_GetReceivedPDU( nmea_PDU_t *pgn );

#endif /* NMEA_H_ */
