/*
 * Lab06.c
 *
 * Created: 10/12/2018 12:59:44 PM
 * Author : cdphan
 */ 


#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#define BAUD 9600
#define BAUDRATE ((F_CPU) / (16UL*BAUD)) - 1

void usart_init(); //makes initial preparations
void delayx(double a); //allows the delay function to be "passed" a variable (checkpoint only)
double calc_period(double i); //calculates period of a note; i = half-tones above A4


int main(void)
{
	//initialisation steps
	usart_init();
	
	DDRB = 0b00000000; //all of register B is input (keypad)
	DDRD = 0b11111111; //all of register D is output (keypad)
	DDRC = 0b11111111; //all of register C is output (audio)
	PORTB = 0xFF; //pull up keypad inputs
	PORTD = 0xFF; //set keypad output to logic 1
	PORTC = 0; //set audio output to logic 0
	
	TCCR0A |= (1 << WGM01); //CTC mode
	TCCR0B |= (1 << CS02); //prescale 256
	
	double numpad[4][4] = {{0,1,2,3}, {4,5,6,7}, {8,9,10,11}, {12,13,14,15}};
	const double fulltime = 256.0 * 256.0 * (1.0 / (16 * 1000000)); // total duration of the 256-scaled timer


    while (1) 
    {
		for (int i=4; i<8; i++){ //scanning upper half of data register D
			PORTD &= ~(1 << i); //deactivates one row of output (logic 0)
			for (int j=0; j<4; j++){ //scanning lower half of data register B
				if (!(PINB & (1 << j))){ //if bit j of register B is 0 (pressed)
					double halfperiod = (calc_period(numpad[i-4][j])) / 2.0; //in seconds (used in checkpoint only)
					double fullperiod = calc_period(numpad[i-4][j]); //in seconds
					double percentage = fullperiod / fulltime; //% of the full timer that the halfperiod takes up
					
					OCR0A = (percentage * 256) - 1; //period compare
					OCR0B = (percentage * 128) - 1; //50% duty cycle compare
					
					while (!(PINB & (1 << j))){ //create and maintain sound output while the button is held
						PORTC |= (1 << 4); //on
						while ((TIFR0 & (1 << OCF0B)) == 0){} //wait until OCF0B flag is 1 e.g. duty cycle is ending
						TIFR0 |= (1 << OCF0B); //write logic 1 and clear the flag
						//delayx(halfperiod*1000); //stay on for half the period cycle
						
						PORTC &= ~(1 << 4); //off
						while ((TIFR0 & (1 << OCF0A)) == 0){} //wait until OCF0A flag is 1 e.g. period is ending
						TIFR0 |= (1 << OCF0A); //write logic 1 and clear the flag
						//delayx(halfperiod*1000); //stay off for the other half
					}
				}
			}
			PORTD |= (1 << i); //reactivate the row before deactivating the next
		}
    }
	
	return 0;
}


void usart_init(){
	UBRR0H = (BAUDRATE >> 8); //upper half into UBRR high
	UBRR0L = BAUDRATE; //lower half into UBRR low
	UCSR0B |= (1 << TXEN0); //set this bit in control/status register b for transmitter functionality
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); //set these two bits in control/status register c to 1 for 8-bit data format
}


void delayx (double a){ //effective up to 2 decimal places
	for (int i=0; i<(int)(a*100); i++){
		_delay_ms(.01);
	}
}


double calc_period(double i){
	double ratio = pow(2.0,.0833333);
	double A4 = 440.0;
	double multiplier = pow(ratio,i);
	double period = 1.0 / (A4 * multiplier);
	return period;
}