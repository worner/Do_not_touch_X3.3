/* Do_not_touch_X3.3
 *  main.c
 * 
 * Zadanie programu:
 * 1. Trzecia wersja programu zapalania trzech taśm LED
 * 		w kuchni Mamy.
 *
 * Created on: 15-12-2013
 *     Author: Jacek Wagner
 *   Procesor: ATtiny 13A
 *		Clock: 4,8 MHz bez podziału przez 8
 * +=+=+=+=+=+=+=+=+=+=+=+
 */ GitHub teks testowy
//=== PLIKI NAGŁÓWKOWE ===
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//=== MAKRODEFINICJE PREPROCESORA ===
#define IR_NAD 		(1<<PB0)			// dioda nadawcza IR - WYJście
#define IR_ODB 		(1<<PB1)			// fototranzystor IR - WEJście
#define LED_PORT 	PORTB
#define DATA_DIR	DDRB
#define SZAFA		(1<<PB2)			// taśma LED pod szafką
#define SZAFA_ON	LED_PORT &= ~SZAFA
#define SZAFA_OFF	LED_PORT |= SZAFA
#define OKNO 		(1<<PB3) 			// taśma LED pod parapetem
#define OKNO_ON		LED_PORT &= ~OKNO
#define OKNO_OFF	LED_PORT |= OKNO
#define FLOOR 		(1<<PB4) 			// taśma LED przy podłodze pod szafkami
#define FLOOR_ON	LED_PORT &= ~FLOOR
#define FLOOR_OFF	LED_PORT |= FLOOR
#define REVERSE		0		// wybór trybu kolejności włączania taśm
							// REVERSE = 0 - SZAFA > OKNO > FLOOR > ALL > OFF
							// REVERSE = 1 - ALL > FLOOR > OKNO > SZAFA > OFF


//=== Deklaracje funkcji i zmiennych ===
// flagi
volatile uint8_t szafa_status, okno_status, floor_status, start_status, end_status;
volatile uint8_t pwm_szafa, pwm_okno, pwm_floor;	// liczniki pwm programowych
enum {soft_on, on, soft_off, off};	// typ wyliczeniowy na potrzeby flag

void START_sequence (void);			// sekwencja startowa zawsze przed pierwszym
									// włączeniem pierwszej taśmy
void END_sequence (void);			// całkowite wygaszenie taśm
void OFF_sequence (uint8_t kanal);	// sekwencja wygaszania taśm z użyciem PWM

// ============================================================================
//=== PĘTLA GŁÓWNA MAIN ===
int main (void)
{
	//+++ Inicjalizacje +++
	// TAŚMY LED - sterowanie poziomem niskim
	DATA_DIR |= IR_NAD | SZAFA | OKNO | FLOOR;	// kierunek pinów - WYJścia
	LED_PORT |= SZAFA | OKNO | FLOOR;			// wygaszenie taśm

	// FOTOTRANZYSTOR
	DATA_DIR &= ~IR_ODB;			// kierunek pinu - WEJściowy
	LED_PORT |= IR_ODB;				// podciąganie do Vcc

	// PRZERWANIA - INT0 dla FOTOTRANZYSTORA
	MCUCR |= (1<<ISC01);			// przerwanie INT0 od zbocza opadającego
	GIMSK |= (1<<INT0);				// włączenie przerwania zewnętrznego INT0

	// PRZERWANIA -  TRYB CTC TIMERA_0A dla OBSŁUGI PWM
	// podział zegara: (4 800 000/2*64/4)-1 = 1 kHz = ~ 500 us
	TCCR0A |= (1<<WGM01);			// MODE 2 - tryb CTC
	TCCR0B |= (1<<CS01) | (1<<CS00);// preskaler = 64
	OCR0A = 4;						// dodatkowy podział przez 4
	TIMSK0 |= (1<<OCIE0A);			// włączenie przerwań CTC dla Timer_0A

	// PRZERWANIA - TRYB CTC TIMERA_0B dla DIODY NADAWCZEJ IR
	TIMSK0 |= (1<<OCIE0B);			// włączenie przerwań CTC dla Timer_0B
									// podział w obsłudze przerwania
	sei ();						// włączenie globalnego zezwolenia na przerwania

// ============================================================================
// 						=== PĘTLA GŁÓWNA PROGRAMU ===
// ============================================================================
	//START_sequence ();
	while (1)
	{
		if (start_status == on) START_sequence ();
		if (szafa_status == on) SZAFA_ON;
		else if (szafa_status == soft_off) OFF_sequence (SZAFA);
		else if (szafa_status == off) SZAFA_OFF;
		if (okno_status == on) OKNO_ON;
		else if (okno_status == soft_off) OFF_sequence (OKNO);
		else if (okno_status == off) OKNO_OFF;
		if (floor_status == on) FLOOR_ON;
		else if (floor_status == soft_off) OFF_sequence (FLOOR);
		else if (floor_status == off) FLOOR_OFF;
		if (end_status == soft_off) END_sequence ();
	}
}

// +++ END MAIN +++

// === DEFINICJE FUNKCJI ===
void start_sequence (void)			// sekwencja startowa przed każdym
{									// włączeniem pierwszej taśmy
	TIMSK0 &= ~(1<<OCIE0A);
	SZAFA_ON;
	_delay_ms (100);
	SZAFA_OFF;
	OKNO_ON;
	_delay_ms (100);
	OKNO_OFF;
	FLOOR_ON;
	_delay_ms (100);
	FLOOR_OFF;
	_delay_ms (100);
	TIMSK0 |= (1<<OCIE0A);
	start_status = off;				// zerowanei flagi sekwencji startowej
}
// ============================================================================
void OFF_sequence (uint8_t kanal)	// wygaszanie z użyciem PWM - SOFT OFF
{
	uint8_t i=255;
	switch (kanal)
	{
	case (SZAFA):
			while (--i>0)
			{
				pwm_szafa = i;
				if (i>50) _delay_ms (5);
				else _delay_ms (20);
			}
	if (i == 0) szafa_status = off;			// zerowanie flagi zezwolenia
	break;
	case (OKNO):
			while (--i>0)
			{
				pwm_okno = i;
				if (i>50) _delay_ms (5);
				else _delay_ms (20);
			}
	if (i == 0) okno_status = off;					// zerowanie flagi zezwolenia
	break;
	case (FLOOR):
			while (--i>0)
			{
				pwm_floor = i;
				if (i>50) _delay_ms(5);
				else _delay_ms (20);
			}
	if (i == 0) floor_status = off;					// zerowanie flagi zezwolenia
	break;
	}
}
// ============================================================================
void END_sequence (void)	// funkcja końcowa gaszenie wszystkich taśm
{
	uint8_t i = 255;			// licznik dla PWM
	szafa_status = soft_off;
	okno_status = soft_off;
	while (--i > 0)
	{
		pwm_szafa = i;
		pwm_okno = i;
		if (i>50) _delay_ms (5);
		else _delay_ms (20);
	}
	if (i == 0)
	{
		szafa_status = off;
		okno_status = off;
		end_status = off;
	}
}

/*******************************************************************************
 * 					*** PROCEDURY OBSŁUGI PRZERWAŃ	***						   *
 *******************************************************************************
*/
// 					=== EXT INT0 - FOTOTRANZYSTOR ===
ISR (INT0_vect)
{
	static uint8_t imp;					// licznik impulsów
	imp++;
	if (imp > 0 && imp < 2)				// SZAFA_ON
	{
		szafa_status = on;
		start_status = on;
	}
	else if (imp > 2 && imp < 4) 		// SZAFA_OFF, OKNO_ON
	{
		okno_status = on;
		if (OKNO_ON) szafa_status = soft_off;
	}
	else if (imp > 4 && imp < 6)		// OKNO_OFF, FLOOR_ON
	{
		floor_status = on;
		if (FLOOR_ON) okno_status = soft_off;
	}
	else if (imp > 6 && imp < 8)		// SZAFA_ON, OKNO_ON, FLOOR_OFF
	{
		szafa_status = on;
		okno_status = on;
		if ((SZAFA_ON) && (OKNO_ON)) floor_status = soft_off;
	}
	else if (imp > 8 && imp < 10) 		// OFF wszystkie taśmy
	{
		end_status = soft_off;
	}

	if (imp == 10) imp = 0;				// zerowanie licznika impulsów
}
// ============================================================================
// 				=== COMPARE MATCH CTC TIMER_0A - OBSŁUGA PWM ===
ISR (TIM0_COMPA_vect)
{
	static uint8_t counter;
	if (szafa_status == soft_off)
	{
		if (counter>= pwm_szafa)	SZAFA_OFF; 	else SZAFA_ON;
	}
	if (okno_status == soft_off)
	{
		if (counter>= pwm_okno) 	OKNO_OFF; 	else OKNO_ON;
	}
	if (floor_status == soft_off)
	{
		if (counter>= pwm_floor) 	FLOOR_OFF;	else FLOOR_ON;
	}
	counter++;
}
// ============================================================================
// 			=== COMPARE MATCH CTC TIMER_0B - DIODA NADAWCZA IR ===
ISR (TIM0_COMPB_vect)
{
	static uint16_t impuls_ir;		// dodatkowy podział częstotliwości przerwania
	if (++impuls_ir > 6000)			// z TIMER_0B przez 6000, co daje f = ~1.56 Hz,
	{								// T=640 ms, -Width = ~210 ms
		LED_PORT &= ~(IR_NAD);		// włączenie diody IR
		if (impuls_ir == 9000)
		{
			LED_PORT |= (IR_NAD);	// wyłączenie diody - szerokość impulsu
			impuls_ir = 0;			// około 250 ms

		}
	}
}
// ============================================================================

/*
Program:     902 bytes (88.1% Full)
(.text + .data + .bootloader)

Data:         12 bytes (18.8% Full)
(.data + .bss + .noinit)
 */
