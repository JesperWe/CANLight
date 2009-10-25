/*
 * nmea.c
 *
 *  Created on: 2009-jun-01
 *      Author: Jesper W
 */

#include "hw.h"
#include "led.h"
#include "nmea.h"
#include "events.h"


//---------------------------------------------------------------------------------------------
// Globals

nmea_MsgBuffer_t	nmea_MsgBuf[nmea_NO_MSG_BUFFERS] __attribute__((space(dma),aligned(nmea_NO_MSG_BUFFERS*nmea_MSG_BUFFER_WORDS*2)));
nmea_MsgBuffer_t	nmea_TxQueue[nmea_NO_MSG_BUFFERS];
unsigned char		nmea_TxQueueHead;
unsigned char		nmea_TxQueueTail;
unsigned char		nmea_TxQueueFull;

unsigned char 		CommandedAddress;

unsigned char		CommandedAddressSource;
unsigned char 		CommandedAddressName[nmea_MSG_BUFFER_WORDS];

unsigned long 		ContentionWaitTime;
unsigned char 		nmea_CA_Address = 0x55;
nmea_FLAG    		nmea_Flags;
nmea_MsgBuffer_t	inMessage;
nmea_PGN_t 			inPGN, outPGN;

nmea_CA_NAME_t		nmea_CA_Name;

WORD				nmea_HW_EN_Reg_Addr;
WORD				nmea_HW_EN_Reg_Bit;
WORD				nmea_HW_Rate_Reg_Addr;
WORD				nmea_HW_Rate_Reg_Bit;

//---------------------------------------------------------------------------------------------
// Set up CAN Module for baud rate = 250kBit, 12 TQ with sample point at 80% of bit time.
// (Based on Fosc = 7.38MHz)

void nmea_Initialize() {

	nmea_TxQueueHead = nmea_TxQueueTail = nmea_TxQueueFull = 0;

	// Build CA NAME.

	nmea_CA_Name.arbitrary_addr = hw_Config.data[1];
	nmea_CA_Name.industry_group = hw_Config.data[2];
	nmea_CA_Name.vehicle_syst = hw_Config.data[3];
	nmea_CA_Name.vehicle_syst_inst = 0;
	nmea_CA_Name.reserved = 0;
	nmea_CA_Name.function = hw_Config.data[4];
	nmea_CA_Name.function_inst = hw_Config.data[5];
	nmea_CA_Name.ecu_inst = 5;
	nmea_CA_Name.manufacturer = hw_Config.data[6];
	nmea_CA_Name.identity = hw_Config.nmeaIdentityNumber;

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

	// Set up one filter and mask, accept only PGN # 65089, Lighing Command.

	C1FEN1bits.FLTEN0=1;			// Filter 0 active.
	C1FMSKSEL1bits.F0MSK=0;		// Select Acceptance Filter Mask 0 for Acceptance Filter 0.

	C1CTRL1bits.WIN = 0x1;
	C1RXM0SID = 0x07E3;				// The 8 PDUFormat bits...
	C1RXF0SID = 0x07E2;				// ...are equal to 0XFE (PDUFormat = 254).
	C1RXM0EID = 0xFF00;				// The 8 PDUSpecific bits...
	C1RXF0EID = 0x4100;				// .. are equal to PDUSpecific = 65.
	C1RXM0SIDbits.MIDE = 1;			// We don't want to accept standard frames.
	C1RXF0SIDbits.EXIDE = 1;
	C1BUFPNT1bits.F0BP = 0x1;		// Store in buffer 1.
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

	hw_WritePort( hw_CAN_EN, 1);	// Go on bus;
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

unsigned char nmea_SendEvent( event_t *event )
{
	unsigned char status;
	nmea_MakePGN( 0, nmea_LIGHTING_COMMAND, 8 );
	outPGN.data[0] = event->type;
	outPGN.data[1] = event->data;
	outPGN.data[2] = (event->atTimer&0xFF00) >> 8;
	outPGN.data[3] = event->atTimer&0x00FF;
	outPGN.data[4] = event->ctrlDev;
	outPGN.data[5] = event->ctrlFunc;
	outPGN.data[6] = event->ctrlEvent;
	status = nmea_SendMessage();

	if( loopbackEnabled ) {
		events_Push(
			e_NMEA_MESSAGE,
			nmea_LIGHTING_COMMAND,
			hw_DeviceID,
			event->ctrlFunc,
			event->ctrlEvent,
			event->data,
			event->atTimer
		);
	}

	return status;
}

//---------------------------------------------------------------------------------------------

unsigned char nmea_SendMessage()
{
	unsigned long SID, SIDExt;

	nmea_MsgBuffer_t msg;

	if( nmea_Flags.CannotClaimAddress ) 	return nmea_CANNOTTRANSMIT;

	// Build local copy of message

	memset( &msg, 0, nmea_MSG_BUFFER_BYTES );

	SID = outPGN.Priority << 8;
	SID = SID | (outPGN.PDUFormat & 0xFC) >> 2;

	SIDExt = ((unsigned long)(outPGN.PDUFormat & 0x03)) << 16;
	SIDExt = SIDExt | ((unsigned long)outPGN.PDUSpecific) << 8;
	SIDExt = SIDExt | outPGN.SourceAddress;


	msg[0] = (SID << 2) | nmea_NORMAL_MSG | nmea_EXTENDED_ID;
	msg[1] = (SIDExt & 0x3FFC0) >> 6;
	msg[2] = ( (SIDExt & 0x3F) << 10 ) | nmea_NORMAL_MSG | outPGN.bytes;

	memcpy( (&msg[3]), outPGN.data, outPGN.bytes );

	if( nmea_TX_IN_PROGRESS ) {

		if( nmea_TxQueueFull ) return nmea_TRANSMITTER_BUSY;

		memcpy( &(nmea_TxQueue[nmea_TxQueueTail]), &msg, nmea_MSG_BUFFER_BYTES );

		nmea_TxQueueTail++;
		nmea_TxQueueTail = nmea_TxQueueTail % nmea_NO_MSG_BUFFERS;

		if( nmea_TxQueueTail == nmea_TxQueueHead ) nmea_TxQueueFull = 1;

		return nmea_SUCCESS;
	}

	// Put completed message in transmit DMA buffer and send it.

	memcpy( &(nmea_MsgBuf[0]), &msg, nmea_MSG_BUFFER_BYTES );

	nmea_TX_IN_PROGRESS = 1;

	return nmea_SUCCESS;
}


//---------------------------------------------------------------------------------------------
// Build NMEA PGN

void nmea_MakePGN(
		unsigned short pgn_priority,
		unsigned short pgn_no,
		unsigned short msg_bytes ) {

	// Build NMEA PGN

	outPGN.PGN				= pgn_no;
	outPGN.PDUFormat 		= (pgn_no & 0xFF00) >> 8;
	outPGN.PDUSpecific 		= pgn_no & 0x00FF;
	outPGN.Datapage			= 0;
	outPGN._Reserved		= 0;
	outPGN.Priority			= pgn_priority;
	outPGN.SourceAddress 	= nmea_CA_Address;
	outPGN.bytes		 	= msg_bytes;
}

//---------------------------------------------------------------------------------------------

void nmea_GetReceivedPGN( nmea_PGN_t *pgn ) {
	unsigned long SID, SIDExt, EID;

	SID = (inMessage[0] & 0x1FFC) >> 2;
	SIDExt = ((unsigned long)inMessage[1]) << 6;
	SIDExt |= (inMessage[2] & 0xFC00) >> 10;
	EID = (SID << 18) | SIDExt;

	pgn->Priority = (SID & 0300) >> 8;
	pgn->PDUFormat = (SID & 0x003F) << 2 | ((EID & 0x30000) >> 16);
	pgn->PDUSpecific = (EID & 0xFF00) >> 8;
	pgn->PGN = (pgn->PDUFormat << 8) | pgn->PDUSpecific;
	pgn->Priority = (SID & 0300) >> 8;
	pgn->SourceAddress = EID & 0xFF;

	pgn->bytes = inMessage[2] & 0x000F;
	memcpy( pgn->data, (&inMessage[3]), pgn->bytes );
}


//---------------------------------------------------------------------------------------------
// CANBus event interrupt.

void __attribute__((interrupt, no_auto_psv)) _C1Interrupt( void ) {
	IFS2bits.C1IF = 0;

	// IVRIF Invalid Message Received
	if(C1INTFbits.IVRIF) {
		C1INTFbits.IVRIF = 0;
		nmea_TX_IN_PROGRESS = 0;	// Abort!
		return;
	}

	if(C1INTFbits.WAKIF) C1INTFbits.WAKIF = 0;

	if(C1INTFbits.TBIF) {
		C1INTFbits.TBIF = 0;

		// Any messages to send in the queue?

		if( (nmea_TxQueueTail != nmea_TxQueueHead) || nmea_TxQueueFull ) {
			memcpy( &(nmea_MsgBuf[0]), &(nmea_TxQueue[nmea_TxQueueHead]), nmea_MSG_BUFFER_BYTES );
			nmea_TxQueueHead++;
			nmea_TxQueueHead = nmea_TxQueueHead % nmea_NO_MSG_BUFFERS;
			nmea_TxQueueFull = 0;
			nmea_TX_IN_PROGRESS = 1;
		}
	}

	// Any received messages?

    if(C1INTFbits.RBIF) {
		unsigned short timer;
		C1INTFbits.RBIF = 0;

		// Retrieve data from buffer 1 and mark it as free.
		memcpy( &inMessage, &(nmea_MsgBuf[1]), nmea_MSG_BUFFER_BYTES );
		C1RXFUL1bits.RXFUL1 = 0;
		C1RXOVF1bits.RXOVF1 = 0;

		nmea_GetReceivedPGN( &inPGN );
		timer = inPGN.data[2];
		timer = timer<<8 | inPGN.data[3];

		if( inPGN.PGN == nmea_MAINTAIN_POWER ) return;

		events_Push( e_NMEA_MESSAGE, inPGN.PGN, 
			inPGN.data[4], inPGN.data[5], inPGN.data[6],
			inPGN.data[1], timer );
	}
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
