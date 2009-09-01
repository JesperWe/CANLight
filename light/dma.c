#include <libpic30.h>
#include <p24Hxxxx.h>

#define FCY 3685000L  
#define CYCLES_PER_MS ((unsigned long)(FCY * 0.001)) 
#define DELAY_MS(ms)  __delay32(CYCLES_PER_MS * ((unsigned long) ms));
#define FILTER_LENGTH 16

_FOSCSEL( FNOSC_FRC )
_FWDT( FWDTEN_OFF )
_FICD( ICS_PGD1 )

struct {
	unsigned int Adc1Ch0[8];
	unsigned int Adc1Ch1[8];
	unsigned int Adc1Ch2[8];
	unsigned int Adc1Ch3[8];
} BufferA __attribute__((space(dma)));

struct {
	unsigned int Adc1Ch0[8];
	unsigned int Adc1Ch1[8];
	unsigned int Adc1Ch2[8];
	unsigned int Adc1Ch3[8];
} BufferB __attribute__((space(dma)));


void initDma0(void){
	DMA0CONbits.AMODE = 2;	// Configure DMA for Peripheral indirect mode
	DMA0CONbits.MODE  = 2; 	// Configure DMA for Continuous Ping-Pong mode
	DMA0PAD = 0x0300;		// Point DMA to ADC1BUF0
	DMA0CNT = 31;			// 32 DMA request (4 buffers, each with 8 words)
	DMA0REQ = 13;			// Select ADC1 as DMA Request source
	DMA0STA = __builtin_dmaoffset(&BufferA);
	DMA0STB = __builtin_dmaoffset(&BufferB);
	IFS0bits.DMA0IF = 0;	//Clear the DMA interrupt flag bit
    IEC0bits.DMA0IE = 1;	//Set the DMA interrupt enable bit
	DMA0CONbits.CHEN=1;		// Enable DMA
}


unsigned int DmaBuffer = 0;

void __attribute__((__interrupt__)) _DMA0Interrupt(void) {

	if(DmaBuffer == 0) {
		ProcessADCSamples(BufferA.Adc1Ch0);
		ProcessADCSamples(BufferA.Adc1Ch1);
		ProcessADCSamples(BufferA.Adc1Ch2);
		ProcessADCSamples(BufferA.Adc1Ch3);
	} else {
		ProcessADCSamples(BufferB.Adc1Ch0);
		ProcessADCSamples(BufferB.Adc1Ch1);
		ProcessADCSamples(BufferB.Adc1Ch2);
		ProcessADCSamples(BufferB.Adc1Ch3);
	}

	DmaBuffer ^= 1;

	IFS0bits.DMA0IF = 0; //Clear the DMA0 Interrupt Flag
}


void InitADC(void)
{
	AD1CON1bits.ADON = 0;
	AD1CON1 = 0x00E0;	// Idle=Stop, 10bit, unsigned, Auto conversion.
	AD1CON2 = 0x003C;	// AVdd/AVss, No scanning, CH0 only, fill buffer at 0x0, 16 samples/interrupt
	AD1CON3 = 0x0F04;	// System clock, Ts = 31xTad, Tad=4xTcy.
	AD1CON4 = 0x0000;	// 1 word buffer.
	AD1CHS0 = 0x0404;	// AN4 input measured to AVss.
	AD1CHS123 = 0;
	AD1CSSL = 0x0000;	// No scanning.
	AD1PCFGL = 0x1FEF;	// AN4 only.

	IFS0bits.AD1IF = 0;
}

void ProcessADCSamples( unsigned int

int main (void)
{
	int count, readings[FILTER_LENGTH];
	short *ADC16Ptr;
	int ADCValue = 0;

	InitADC();
	TRISC = 0x000;

	while (1)
	{
		ADCValue = 0;
		ADC16Ptr = &ADC1BUF0;
		IFS0bits.AD1IF = 0;

		AD1CON1bits.ASAM = 1;

		AD1CON1bits.ADON = 1;
		while (!IFS0bits.AD1IF);
		AD1CON1bits.ADON = 0;

		AD1CON1bits.ASAM = 0;

		for( count=0; count<FILTER_LENGTH; count++ ) {
			ADCValue += *ADC16Ptr;
			readings[count] = *ADC16Ptr;
			ADC16Ptr++;
		}
		ADCValue = ADCValue / FILTER_LENGTH;
		LATC = ADCValue;
	}
}
