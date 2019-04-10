/*
 * Verzoegerung.c
 *
 * Created: 04.04.2019 22:50:55
 * Author : Nico
 */ 

#ifndef F_CPU
#define F_CPU 1000000
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/power.h>    // Power management
#include <avr/interrupt.h>

#define STATE_OFF	0
#define STATE_IDLE	1
#define STATE_WAIT	2
#define STATE_ON	3

#define SWITCH_DELAY	5000	/* Delay time in ms */

static volatile uint8_t		g_bySwitch = 0;
static volatile uint32_t	g_dwTick = 0;

void mcu_sleep(void)
{
	MCUCR |= (1 <<SM1);
	power_all_disable();
	sleep_enable();
	sleep_cpu();
}

int main(void)
{
	static uint8_t bySwitchState = STATE_IDLE;
	
	PORTA = 0;
	
	DDRB |= (1 << PB4);						/* Main LED */
	DDRA |= (1 << PA0) | (1 << PA1);		/* Relay outputs */
	
	GIMSK |= (1 << INT0);					/* Enable Pin Interrupt */
	
	TCCR1B = 0x03;							/* Prescaler f_cpu/4 = 250 kHz */
	TCCR1B |= (1 << CTC1);
	OCR1C = 249;							/* 250 kHz / 250 => 1 ms */
	
	TIMSK |= (1 << TOIE1);					/* Enable Timer overflow Interrupt */
	sei();
	
    while (1) 
    {		
		switch(bySwitchState)
		{
		case STATE_IDLE:					/* Run once after power on */
			bySwitchState = STATE_OFF;			
			g_bySwitch = 0;
			mcu_sleep();
			break;
		case STATE_OFF:						/* Systemn off -> Switch main relay on */
			if(g_bySwitch == 1)
			{
				g_bySwitch = 0;
				PORTA |= (1 << PA0);
				PORTB |= (1 << PB4);
				g_dwTick = 0;
				g_bySwitch = 0;
				bySwitchState = STATE_WAIT;
			}			
			break;
		case STATE_WAIT:					/* Wait x Seconds before turning on Speaker Relays */			
			 if(g_bySwitch == 1)			/* If Button was pressed to turn system off BEFORE delay exceeded */
			 {		
				 g_dwTick = 0;		 
				 bySwitchState = STATE_ON;
				 g_bySwitch = 0;
			 }
		
			if(g_dwTick >= SWITCH_DELAY)	/* Switch on Speaker Relays */
			{	
				g_dwTick = 0;			
				bySwitchState = STATE_ON;
				PORTA |= (1 << PA1);
				mcu_sleep();
			}
			break;
		case STATE_ON:						/* System on -> Turn off */				
			PORTA = 0;
			PORTB &= ~(1 << PB4);
			g_bySwitch = 0;
			g_dwTick = 0;
			bySwitchState = STATE_OFF;
			mcu_sleep();
			break;
		default:
			bySwitchState = STATE_OFF;
			PORTA = 0;
			mcu_sleep();
			break;
		}	 
    }
}

ISR(TIMER1_OVF1_vect)
{
	g_dwTick++;
}

ISR(INT0_vect)
{	
	if(g_dwTick > 100)
	{		
		g_bySwitch = 1;
	}	
}



