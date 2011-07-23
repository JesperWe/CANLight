/*
 * nmea.c
 *
 *  Created on: 2009-jun-01
 *      Author: Jesper W
 */

#include "hw.h"
#include "config.h"
#include "events.h"
#include "led.h"
#include "nmea.h"
#include "events.h"
#include "schedule.h"

//---------------------------------------------------------------------------------------------
// Globals

nmea_MsgBuffer_t	nmea_MsgBuf[nmea_NO_MSG_BUFFERS] __attribute__((space(dma),aligned(nmea_NO_MSG_BUFFERS*nmea_MSG_BUFFER_WORDS*2)));
nmea_MsgBuffer_t	nmea_TxQueue[nmea_NO_MSG_BUFFERS];
unsigned char		nmea_TxQueueHead;
unsigned char		nmea_TxQueueTail;
unsigned char		nmea_TxQueueFull;

unsigned char		nmea_TPMessage_Complete;
unsigned char		nmea_TPMessage_LastPackage;
unsigned char		nmea_TPMessage_Error;
short 				nmea_TPMessage_Size;
long				nmea_TPMessage_PGN;
short 				nmea_TPMessage_Bytes;

unsigned char 		CommandedAddress;

unsigned char		CommandedAddressSource;
unsigned char 		CommandedAddressName[nmea_MSG_BUFFER_WORDS];

unsigned long 		ContentionWaitTime;
unsigned char 		nmea_CA_Address = 0x55;
nmea_FLAG    		nmea_Flags;
nmea_MsgBuffer_t	inMessage;
nmea_PDU_t 			inPDU, nmea_OutgoingPDU;

nmea_CA_NAME_t		nmea_CA_Name;

WORD				nmea_HW_EN_Reg_Addr;
WORD				nmea_HW_EN_Reg_Bit;
WORD				nmea_HW_Rate_Reg_Addr;
WORD				nmea_HW_Rate_Reg_Bit;

unsigned short		nmea_msgCounter = 0;
unsigned short		nmea_overflowCounter = 0;
unsigned short		nmea_invalidMsgCounter = 0;
unsigned			nmea_LastICode = 0;


//---------------------------------------------------------------------------------------------
// Set up CAN Module for baud rate = 250kBit, 12 TQ with sample point at 80% of bit time.
// (Based on Fosc = 7.38MHz)

void nmea_Initialize() {

	nmea_TxQueueHead = nmea_TxQueueTail = nmea_TxQueueFull = 0;

	// Build CA NAME.

	hw_Config = (hw_Config_t*) hw_ConfigData;

	nmea_CA_Name.arbitrary_addr = hw_Config->nmeaArbitraryAddress;
	nmea_CA_Name.industry_group = hw_Config->nmeaIndustryGroup;
	nmea_CA_Name.vehicle_syst = hw_Config->nmeaVehicleSystem;
	nmea_CA_Name.vehicle_syst_inst = 0;
	nmea_CA_Name.reserved = 0;
	nmea_CA_Name.function = hw_Config->nmeaFunction;
	nmea_CA_Name.function_inst = hw_Config->nmeaFunctionInstance;
	nmea_CA_Name.ecu_inst = 5;
	nmea_CA_Name.manufacturer = hw_Config->nmeaManufacturerCode;
	nmea_CA_Name.identity = hw_Config->nmeaIdentityNumber;

	// Go to config mode.

	IEC2bits.C1IE = 0;
	C1CTRL1bits.REQOP = 4;
	while( C1CTRL1bits.OPMODE != 4 );

	C1CTRL1bits.CSIDL = 1;

	C1CFG1bits.SJW = 0;
	C1CFG1bits.BRP = 0;

	//C1CFG2bits.WAKFIL = 1;
	C1CFG2bits.PRSEG = 0;
	C1CFG2bits.SEG1PH = 5;
	C1CFG2bits.SEG2PH = 1;
	C1CFG2bits.SAM = 0;

	C1FCTRLbits.DMABS=0;		// 4 Buffers.

	C1TR01CONbits.TXEN0=1;			// Buffer 0 is a Transmit Buffer.
	C1TR01CONbits.TX0PRI=0x2; 		// Message Buffer 0 Priority Level = High.

	// Upper byte of PGN, called PF:
	//	PF(7:2) maps to SID(5:0) or EID(23:18).
	//  PF(1:0) maps to EID(17:16).
	//
	// Lower byte of PGN, called PS:
	//  PS(7:0) maps to EID(15:8)
	//
	// EID(7:0) is the Source Address.

	// One filter accept only PGN # 65089, Lighting Command.

	C1FEN1bits.FLTEN0=1;			// Filter 0 active.
	C1FMSKSEL1bits.F0MSK=0;			// Select Acceptance Filter Mask 0 for Acceptance Filter 0.

	C1CTRL1bits.WIN = 0x1;
	C1RXM0SID = 0; //0x07E3;				// The 8 PDUFormat bits...
	C1RXF0SID = 0x07E2;				// ...are equal to 0XFE (PDUFormat = 254).
	C1RXM0EID = 0; //0xFF00;				// The 8 PDUSpecific bits...
	C1RXF0EID = 0x4100;				// .. are equal to PDUSpecific = 65.
	C1RXM0SIDbits.MIDE = 1;			// We don't want to accept standard frames.
	C1RXF0SIDbits.EXIDE = 1;
	C1BUFPNT1bits.F0BP = 0x1;		// Store in buffer 1.

	// One filter for config updates, PGN 60416/0xEC00/CM_BAM and 60160/0xEB00/DATATRANSFER
	// So the mask is 0xF8FF, value 0xE800

	C1FEN1bits.FLTEN1=1;			// Filter 1 active.
	C1FMSKSEL1bits.F1MSK=1;			// Select Acceptance Filter Mask 1 for Acceptance Filter 1.

	C1RXM1SID = 0x1F03;				// The 8 PDUFormat bits.
	C1RXF1SID = 0x1D00;				//
	C1RXM1EID = 0xFF00;				// The 8 PDUSpecific bits.
	C1RXF1EID = 0x0000;				//
	C1RXM1SIDbits.MIDE = 1;			// We don't want to accept standard frames.
	C1RXF1SIDbits.EXIDE = 1;
	C1BUFPNT1bits.F1BP = 0x1;		// Store in buffer 1.

	C1CTRL1bits.WIN = 0x0;


	// DMA Initialization for ECAN1 Transmit.

	DMACS0 = 0;
	DMA0CONbits.SIZE = 0x0;
	DMA0CONbits.DIR = 1;
	DMA0CONbits.AMODE = 0x2;
	DMA0PAD = (unsigned short)(&C1TXD);
	DMA0CNT = 7;
	DMA0REQ = 70;					// ECAN 1 Transmit
	DMA0STA = __builtin_dmaoffset(nmea_MsgBuf);
	IEC0bits.DMA0IE = 1;
	DMA0CONbits.CHEN = 1;

	// DMA Initialization for ECAN1 Receive.

	DMACS0 = 0;
	DMA2CON = 0x0020;
	DMA2PAD = (unsigned short)(&C1RXD);
	DMA2CNT = 7;
	DMA2REQ = 34;					// ECAN 1 Receive
	DMA2STA = __builtin_dmaoffset(nmea_MsgBuf);
	IEC1bits.DMA2IE = 1;
	DMA2CONbits.CHEN = 1;

	IEC2bits.C1IE = 1;
	C1EC = 0;
	C1INTF = 0;
	C1INTE = 0xEF;


	// Go to normal mode.

	C1CTRL1bits.REQOP = 0;
    while( C1CTRL1bits.OPMODE != 0 );

	memset( hw_1kBuffer, 0xFF, sizeof(hw_1kBuffer) );

	hw_WritePort( hw_CAN_EN, 1);	// Go on bus;
	nmea_TX_REQUEST_BIT = 0;
}


//---------------------------------------------------------------------------------------------

void nmea_ControllerMode( unsigned char mode ) {
	unsigned char modebits = mode & 0x7;
	C1CTRL1bits.REQOP = modebits;
    while( C1CTRL1bits.OPMODE != modebits );
}


//---------------------------------------------------------------------------------------------
// Pack a CA Name from a struct to a byte array, with bytes numbered according
// to the standard documentation (byte 0 is unused).

void nmea_PackName( unsigned char name[], nmea_CA_NAME_t nameFields ) {
	name[8]  = nameFields.arbitrary_addr << 7;
	name[8] |= nameFields.industry_group << 4;
	name[8] |= nameFields.vehicle_syst_inst;
	name[7]  = nameFields.vehicle_syst << 1;
	name[6]  = nameFields.function;
	name[5]  = nameFields.function_inst<<3;
	name[5] |= nameFields.ecu_inst;
	name[4]  = (unsigned char)((nameFields.manufacturer&0x07F8)>>3);
	name[3]  = (unsigned char)((nameFields.manufacturer&0x0007)<<5);
	name[3] |= (unsigned char)((nameFields.identity&0x1F0000)>>16);
	name[2]  = (unsigned char)((nameFields.identity&0x00FF00)>>8);
	name[1]  = (unsigned char) (nameFields.identity&0x0000FF);
}


//---------------------------------------------------------------------------------------------

void nmea_Wakeup() {
	nmea_MakePGN( 0, nmea_MAINTAIN_POWER, 0 );
	nmea_SendMessage();
}


//---------------------------------------------------------------------------------------------

void nmea_Debug( unsigned char data ) {
	nmea_MakePGN( 0, nmea_MAINTAIN_POWER, 1 );
	nmea_OutgoingPDU.data[0] = data;
	nmea_SendMessage();
}


//---------------------------------------------------------------------------------------------

void nmea_Debug2( unsigned char data0, unsigned char data1 ) {
	nmea_MakePGN( 0, nmea_MAINTAIN_POWER, 2 );
	nmea_OutgoingPDU.data[0] = data0;
	nmea_OutgoingPDU.data[1] = data1;
	nmea_SendMessage();
}


//---------------------------------------------------------------------------------------------

unsigned char nmea_SendEvent( event_t *event )
{
	unsigned char status;
	nmea_MakePGN( 0, nmea_LIGHTING_COMMAND, 8 );
	nmea_OutgoingPDU.data[0] = event->groupId;
	nmea_OutgoingPDU.data[1] = event->data;
	nmea_OutgoingPDU.data[2] = (event->info&0xFF00) >> 8;
	nmea_OutgoingPDU.data[3] = event->info&0x00FF;
	nmea_OutgoingPDU.data[4] = event->ctrlDev;
	nmea_OutgoingPDU.data[5] = event->ctrlPort;
	nmea_OutgoingPDU.data[6] = event->ctrlEvent;
	status = nmea_SendMessage();

	if( loopbackEnabled ) {
		events_Push(
			e_NMEA_MESSAGE, nmea_LIGHTING_COMMAND,
			event->groupId, hw_DeviceID,
			event->ctrlPort,
			event->ctrlEvent,
			event->data,
			event->info
		);
	}

	return status;
}

//---------------------------------------------------------------------------------------------
// Utility to send key click events.
// Since they do not occur often we need to make sure the receiver
// has time to wake from sleep by sending a dummy event first.

unsigned char nmea_SendIOEvent( event_t *event ) {
	unsigned char status;
	nmea_Wakeup();
	__delay32(1500000);

	status = nmea_SendEvent( event );
	return status;
}

//---------------------------------------------------------------------------------------------
// Send the message contained in nmea_OutgoingPDU.

unsigned char nmea_SendMessage()
{
	unsigned long SID, SIDExt;

	nmea_MsgBuffer_t msg;

	if( nmea_Flags.CannotClaimAddress ) 	return nmea_CANNOTTRANSMIT;

	// Build local copy of message

	memset( &msg, 0, nmea_MSG_BUFFER_BYTES );

	SID = nmea_OutgoingPDU.Priority << 8;
	SID = SID | (nmea_OutgoingPDU.PDUFormat & 0xFC) >> 2;

	SIDExt = ((unsigned long)(nmea_OutgoingPDU.PDUFormat & 0x03)) << 16;
	SIDExt = SIDExt | ((unsigned long)nmea_OutgoingPDU.PDUSpecific) << 8;
	SIDExt = SIDExt | nmea_OutgoingPDU.SourceAddress;


	msg[0] = (SID << 2) | nmea_NORMAL_MSG | nmea_EXTENDED_ID;
	msg[1] = (SIDExt & 0x3FFC0) >> 6;
	msg[2] = ( (SIDExt & 0x3F) << 10 ) | nmea_NORMAL_MSG | nmea_OutgoingPDU.bytes;

	memcpy( (&msg[3]), nmea_OutgoingPDU.data, nmea_OutgoingPDU.bytes );

	// Turn of Transmit Buffer interrupts while we are messing with the transmit queue.

	C1INTEbits.TBIE = 0;

	if( nmea_TX_REQUEST_BIT ) {

		if( nmea_TxQueueFull ) {
			C1INTEbits.TBIE = 1;
			return nmea_TRANSMITTER_BUSY;
		}

		memcpy( &(nmea_TxQueue[nmea_TxQueueTail]), &msg, nmea_MSG_BUFFER_BYTES );

		nmea_TxQueueTail++;
		nmea_TxQueueTail = nmea_TxQueueTail % nmea_NO_MSG_BUFFERS;

		if( nmea_TxQueueTail == nmea_TxQueueHead ) nmea_TxQueueFull = 1;

		C1INTEbits.TBIE = 1;
		return nmea_TRANSMIT_QUEUED;
	}

	C1INTEbits.TBIE = 1;

	// Put completed message in transmit DMA buffer and send it.

	memcpy( &(nmea_MsgBuf[0]), &msg, nmea_MSG_BUFFER_BYTES );

	nmea_TX_REQUEST_BIT = 1;

	return nmea_SUCCESS;
}


//---------------------------------------------------------------------------------------------
// Build NMEA PGN

void nmea_MakePGN(
		unsigned short pgn_priority,
		unsigned short pgn_no,
		unsigned short msg_bytes ) {

	// Build NMEA PGN

	nmea_OutgoingPDU.PGN				= pgn_no;
	nmea_OutgoingPDU.PDUFormat 		= (pgn_no & 0xFF00) >> 8;
	nmea_OutgoingPDU.PDUSpecific 		= pgn_no & 0x00FF;
	nmea_OutgoingPDU.Datapage			= 0;
	nmea_OutgoingPDU._Reserved		= 0;
	nmea_OutgoingPDU.Priority			= pgn_priority;
	nmea_OutgoingPDU.SourceAddress 	= nmea_CA_Address;
	nmea_OutgoingPDU.bytes		 	= msg_bytes;
}

//---------------------------------------------------------------------------------------------

void nmea_GetReceivedPDU( nmea_PDU_t *pdu ) {
	unsigned long SID, SIDExt, EID;

	SID = (inMessage[0] & 0x1FFC) >> 2;
	SIDExt = ((unsigned long)(inMessage[1])) << 6;
	SIDExt |= (inMessage[2] & 0xFC00) >> 10;
	EID = (SID << 18) | SIDExt;

	pdu->PDU = EID;

	pdu->bytes = inMessage[2] & 0x000F;
	memcpy( pdu->data, (&inMessage[3]), pdu->bytes );

	pdu->PGN = (pdu->PDUFormat << 8) | pdu->PDUSpecific;
}


//---------------------------------------------------------------------------------------------
// Transmit a multi-package message.
// It is the callers duty to keep message length less than the maximum allowed 7*255 = 1785 bytes.

void nmea_SendMultipacket( unsigned char *msgBuffer, unsigned short msgLength, long containedPGN ) {
	unsigned short noPackets;
	unsigned short byteOffset;
	unsigned char packetCount;
	unsigned char bytesLeft;
	unsigned char status;
	unsigned char i;

	noPackets = msgLength / 7;
	if( msgLength % 7 ) noPackets++;

	// First tell the bus we are about to transmit a multi-packet message.

	nmea_Wakeup();
	nmea_MakePGN( 1, nmea_CM_BAM, 8 );

	nmea_OutgoingPDU.data[0] = 0x20; // TP.CM_BAM
	nmea_OutgoingPDU.data[1] = (msgLength & 0xFF00) >> 8;
	nmea_OutgoingPDU.data[2] = msgLength & 0x00FF;
	nmea_OutgoingPDU.data[3] = noPackets;
	nmea_OutgoingPDU.data[4] = 0xFF; // Reserved
	nmea_OutgoingPDU.data[5] = (containedPGN & 0xFF0000) >> 16;
	nmea_OutgoingPDU.data[6] = (containedPGN & 0x00FF00) >> 8;
	nmea_OutgoingPDU.data[7] =  containedPGN & 0x0000FF;

	status = nmea_SendMessage();

	// Now send data packages.

	packetCount = 1; // Sequence numbers should start at 1.
	byteOffset = 0;
	nmea_MakePGN( 1, nmea_DATATRANSFER, 8 );

	bytesLeft = 8;

	while( byteOffset < msgLength ) {
		for( i=1; i<bytesLeft; i++ ) {
			nmea_OutgoingPDU.data[i] = msgBuffer[byteOffset++];
		}

		nmea_OutgoingPDU.data[0] = packetCount;
		packetCount++;

		do {
			status = nmea_SendMessage();
			if( status == nmea_TRANSMITTER_BUSY ) __delay32( 10000 ); // Yeah yeah...
		} while( status == nmea_TRANSMITTER_BUSY );

		if( packetCount == noPackets ) bytesLeft = msgLength - byteOffset + 1;
	}
}

//---------------------------------------------------------------------------------------------

int nmea_DistributeSettings() {
	hw_ReadConfigFlash();
	nmea_SendMultipacket( hw_1kBuffer, hw_CONFIG_SIZE, nmea_SETTINGS );
	return 0;
}

//---------------------------------------------------------------------------------------------
// ECAN event interrupt. Called both for read and write operations.

void __attribute__((interrupt, no_auto_psv)) _C1Interrupt( void ) {
	IFS2bits.C1IF = 0;
	IEC2bits.C1IE = 0;

	// Stay awake for a while to look for traffic.
	hw_StayAwakeTimer += schedule_SECOND;

	if( C1INTFbits.ERRIF ) {
		C1INTFbits.ERRIF = 0;
		nmea_invalidMsgCounter++;
		nmea_LastICode = C1VECbits.ICODE | 0x80;
	}

	if( C1INTFbits.IVRIF ) {
		C1INTFbits.IVRIF = 0;
		nmea_invalidMsgCounter++;
		nmea_LastICode = C1VECbits.ICODE;
	}

	if( C1INTFbits.WAKIF ) {
		C1INTFbits.WAKIF = 0;
	}

	// Message in TX buffer has been transmitted. Any more messages?

	if( C1INTFbits.TBIF ) {
		C1INTFbits.TBIF = 0;

		if( (nmea_TxQueueTail != nmea_TxQueueHead) || nmea_TxQueueFull ) {
			memcpy( &(nmea_MsgBuf[0]), &(nmea_TxQueue[nmea_TxQueueHead]), nmea_MSG_BUFFER_BYTES );
			nmea_TxQueueHead++;
			nmea_TxQueueHead = nmea_TxQueueHead % nmea_NO_MSG_BUFFERS;
			nmea_TxQueueFull = 0;
			nmea_TX_REQUEST_BIT = 1;
		}
	}

	// Check for overflow.

    if( C1INTFbits.RBOVIF ) {
    	// Oops, we lost stuff!
    	C1INTFbits.RBOVIF = 0;
    	nmea_overflowCounter++;
    }

	// Any received messages?

    if( C1INTFbits.RBIF ) {
		unsigned short timer;
		C1INTFbits.RBIF = 0;

		// Retrieve data from buffer 1 and mark it as free.
		memcpy( &inMessage, &(nmea_MsgBuf[1]), nmea_MSG_BUFFER_BYTES );
		C1RXFUL1bits.RXFUL1 = 0;
		C1RXOVF1bits.RXOVF1 = 0;

		nmea_GetReceivedPDU( &inPDU );
		timer = inPDU.data[2];
		timer = timer<<8 | inPDU.data[3];

		nmea_msgCounter++;

		switch( inPDU.PGN ){

			case nmea_MAINTAIN_POWER: goto done;

			case nmea_LIGHTING_COMMAND: {
				events_Push( e_NMEA_MESSAGE, inPDU.PGN,
					inPDU.data[0], inPDU.data[4], inPDU.data[5], inPDU.data[6],
					inPDU.data[1], timer );
				break;
			}

			case nmea_CM_BAM: {
				if( inPDU.data[0] != 0x20 ) break; // We only implement BAM at the moment, not the other TP.CM parts.
				nmea_TPMessage_Complete = FALSE;
				nmea_TPMessage_Error = FALSE;
				nmea_TPMessage_Size = inPDU.data[1]<<8 | inPDU.data[2];
				nmea_TPMessage_PGN = ((long)inPDU.data[5])<<16 | ((long)inPDU.data[6])<<8 | inPDU.data[7];
				nmea_TPMessage_Bytes = 0;
				nmea_TPMessage_LastPackage = 0;
				break;
			}

			case nmea_DATATRANSFER: {
				short i;

				if( nmea_TPMessage_Error || nmea_TPMessage_Complete ) break;

				nmea_TPMessage_LastPackage++;

				if( inPDU.data[0] != nmea_TPMessage_LastPackage ) {
					// Package was lost. Too bad...
					nmea_TPMessage_Error = TRUE;
					break;
				}

				if( nmea_TPMessage_Bytes > nmea_TPMessage_Size ) {
					// We are getting more data than they said would come!
					nmea_TPMessage_Error = TRUE;
					break;
				}

				for( i=1; i<8; i++ ) {
					hw_1kBuffer[ nmea_TPMessage_Bytes++ ] = inPDU.data[i];
				}

				if( nmea_TPMessage_Bytes >= nmea_TPMessage_Size ) {
					nmea_TPMessage_Complete = TRUE;

					switch( nmea_TPMessage_PGN ) {
						case nmea_LIGHTING_DATA: {
							config_Update( nmea_TPMessage_Size );
							break;
						}
						case nmea_SETTINGS: {

							// Validation

							if( nmea_TPMessage_Size != hw_CONFIG_SIZE ) break;

							if( hw_1kBuffer[0] != hw_CONFIG_MAGIC_WORD_1 // NB Little Endian
									|| hw_1kBuffer[1] != hw_CONFIG_MAGIC_WORD_0 ) break;

							hw_WriteSettingsFlash();

							schedule_Parameter = 2;
							schedule_AddTask( led_TaskComplete, 10 );

							break;
						}
					}

				}

				break;
			}
		}
	}

done: IEC2bits.C1IE = 1;
}


//---------------------------------------------------------------------------------------------
// DMA interrupt handlers.

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
	IFS0bits.DMA0IF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _DMA2Interrupt(void)
{
   IFS1bits.DMA2IF = 0;
}
