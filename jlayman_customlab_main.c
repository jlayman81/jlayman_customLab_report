/* Partner 1 Name & E-mail:Jesse Layman	jlaym003@ucr.edu
* Lab Section:021
* Assignment: Custom Lab
* Exercise Description: Guitar Tuner.
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work. */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "seven_seg.h"

//START TIMER STUFF////////////////////////////
//TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;

//Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1;//Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0;//Current internal count of 1ms ticks.

void TimerOn(){
	//AVR timer/counter controller register TCCR1
	//bit3 = 0: CTC mode (clear timer on compare)
	//bit2bit1bit0=011:pre-scaler/64
	//00001011:0x0B
	//SO, 8 MHz clock or 8,000,000/64 = 125,000 ticks/s
	//Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B= 0x0B;
	
	//AVR output compare register OCR1A.
	//Timer interrupt will be generated when TCNT1==OCR1A
	//We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	//So when TCNT1 register equals 125,
	//1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;
	
	//AVR timer interrupt mask register
	//bit1: OCIEA -- enables compare match interrupt
	TIMSK1 = 0x02;
	
	//Initialize avr counter
	TCNT1 = 0;
	
	//TimerISR will be called every _avr_timer_cntcurr milliseconds
	
	_avr_timer_cntcurr = _avr_timer_M;
	
	//Enable global interrupts: 0x80: 10000000
	SREG|=0x80;
	
}

void TimerOff(){
	//bit3bit1bit0=000: timer off???????????????????????????bit2
	TCCR1B = 0x00;
	
}

void TimerISR(){
	TimerFlag = 1;
}

//In our approach, the C programmer does not touch this ESR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	
	//CPU automatically calls when TCNT == OCR1
	//(every 1 ms per TimerOn settings)
	
	//Count down to 0 rather than up to TOP (results in a more efficient comarison)
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr==0){
		
		//Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr=_avr_timer_M;
	}
}

//Set TimerISR() to tick every M ms
void TimerSet(unsigned long M){
	_avr_timer_M=M;
	_avr_timer_cntcurr=_avr_timer_M;
}
//END TIMER STUFF///////////////////////////////
///////////////////////////////////////////////

//SET UP ADC
void ADC_init(){
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

//Global variables
const unsigned short MAXADC = 0x25F;//use this value to look for large 
						     //spikes in the adc
const unsigned short SAMPLE_SIZE = 1000;
const unsigned char soundSwPeriod = 1000;
unsigned short tempOut =0;
//unsigned short us_sampleAr[1000];
unsigned short frequencyOUTPUT;
unsigned short tempOut;
unsigned short ADCin;
unsigned char OUTPUT;
unsigned char flagDONE;
unsigned char flagOUT;
unsigned char debugLED;

enum sampleStates {sStart,sWait,sSample,sCalculate,sDone} sState;//sample states
void Ticks(){
	static unsigned char soundSW;
	static unsigned short SampleAr[1000];
	static unsigned short i;
	static unsigned short frequency;
	switch(sState){
		case sStart://transitions
		soundSW = 0;
		i = 0;
		frequency = 0;
		//initialize array
		for (unsigned short z = 0; z < SAMPLE_SIZE; z++)
		{SampleAr[z] = 0;}
		sState = sWait;
		break;
		case sWait:
		debugLED= 1;
		if (soundSW>=soundSwPeriod)//sound switch sample
		{
			soundSW =0;
			i=0;
			sState = sSample;
			break;
		}
		else{
			sState = sWait;
		}
		case sSample:
		debugLED= 2;
		if (i<SAMPLE_SIZE)
		{
			i++;
			sState = sSample;
			break;
		}
		else{
			i=0;
			sState = sCalculate;
			break;
		}
		case sCalculate:
		debugLED= 3;
		if (!flagDONE)
		{
			sState = sCalculate;
			break;
		}
		else{
			i= 0;
			tempOut = frequency;
		}
		case sDone:
		debugLED= 4;
		if (!flagOUT)
		{
			sState = sDone;
			break;
		}
		else{
			sState = sWait;
			flagDONE =0;
			//clear array
			while(i<SAMPLE_SIZE){
			SampleAr[i]=0;
			i++;
			}
			i=0;
			break;
		}
	}
	switch(sState){
		case sStart:
		break;
		case sWait:
		debugLED= 5;
		if (ADCin>=MAXADC)//sound switch
		{
			soundSW++;
		}
		break;
		case sSample:
		debugLED= 6;
		if (ADCin>=MAXADC)//ADC sample filter
		{
			SampleAr[i] = 1;
			break;
		}
		else{
			SampleAr[i] = 0;
			break;
			}
		case sCalculate:
		debugLED= 7;
		while (i<=SAMPLE_SIZE)
		{
			if (SampleAr[i]>0)//count the number of high values in the sample array
			{
				frequency++;
			}
			i++;
			flagDONE = 1;
		}
		case sDone:
		debugLED = 8;
		break;
	}
};

enum outputStates {oStart, oWait, oOUT} oState;
void TickOUT(){
	static unsigned short c;
	switch(oState){
		case oStart:
		debugLED = 9;
		c = 0;
		flagOUT = 0;
		oState = oWait;
		break;
		case oWait:
		debugLED = 10;
		if (!flagDONE)
		{
			oState = oWait;
			break;
		}
		else{
			c = 0;
			flagOUT = 0;
			oState = oOUT;
			break;
		}
		case oOUT:
		debugLED = 11;
		if (c<SAMPLE_SIZE*5)
		{
			c++;
			oState = oOUT;
			break;
		}
		else{
			c = 0;
			flagOUT = 1;
			oState = oWait;
			break;
		}
	}
	switch(oState){
		case oStart:
		break;
		case oWait:
		debugLED = 12;
		break;
		case oOUT:
		debugLED = 13;
		frequencyOUTPUT = tempOut;
		
		if ((frequencyOUTPUT>80)&&(frequencyOUTPUT<160))
		{
			OUTPUT = 0x01;
			Write7Seg(SS_E);//test seven seg
			
		}
		if ((frequencyOUTPUT>160)&&(frequencyOUTPUT<200))
		{
			Write7Seg(SS_A);//test seven seg
			OUTPUT =0x02;
			
			
		}
		if ((frequencyOUTPUT>200)&&(frequencyOUTPUT<265)){
		OUTPUT = 0x04;
		Write7Seg(SS_D);//test seven seg
		}
		if((frequencyOUTPUT>265)&&(frequencyOUTPUT<300)){
		OUTPUT = 0x08;
		Write7Seg(SS_G);
		}
		
		break;
	}
	};

int main(void)
{
	//initialize pins
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0x00; PORTD = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	ADC_init();//start ADC
	TimerSet(1);//set timer
	TimerOn();//start timer
	//initialize state machines
	sState = sStart;
	oState = oStart;
	//for testinst button, remove after building output;
	
    while (1) 
    {
		ADCin = ADC;
		Ticks();
		TickOUT();
		
		debugLED = 13;
	    PORTB =OUTPUT;
		PORTD = (char)(tempOut>>8);//debugging
//Wait for timer period
		while (!TimerFlag)
		TimerFlag=0;
    }
}

