#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

// internal function prototypes
void lcdCmduN(char c);												// wm
void lcdCmd(char c);
void lcdData(char c);


void lcdInit()
{
#ifdef LCD_20x4														// wm
	_delay_ms(20);													// min. 15ms
	
	lcdCmduN(0x30);
	_delay_ms(10);													// min. 4.1ms

	lcdCmduN(0x30);
	_delay_ms(1);													// > 100us

	lcdCmduN(0x30);
	_delay_ms(1);
	
	lcdCmduN(0x20);
	_delay_ms(1);

	lcdCmd(0x28);													// NF00, N=1 2 Lines, F=0 5x8 Dots
	_delay_ms(1);

	lcdCmd(0x08);													// Function set, interface 4 bits long 
	_delay_ms(1);

	lcdCmd(0x01);													// Clear Display
	_delay_ms(1);

	lcdCmd(0x06);													// Entry mode set 
	_delay_ms(1);
	
	lcdCmd(0x0C);													// 0000 1DCB, D=1 Display on, C=0 no Cursor, B=0 no blinking 
	_delay_ms(1);	
#else
	// 4 bits mode, 2 lines
	lcdCmd(0x20);
	_delay_ms(10);
	
	lcdCmd(0x20);
	_delay_ms(10);
	
	// cursor shift right, text no shift
	lcdCmd(0x18);
	_delay_ms(20);

	// display on, no cursor, no blink
	lcdCmd(0x0c);
	_delay_ms(20);

	// shift mode
	lcdCmd(0x06);
	_delay_ms(20);

	// home
	lcdCmd(0x02);
	_delay_ms(20);

	// clear display
	lcdCmd(0x01);
	_delay_ms(20);
#endif
}


void lcdCmduN(char c)												// wm use only for LcdInit()
{
	char t;

	t = c & 0xf0;
	PORTB = t;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);
}


void lcdCmd(char c)
{
	char t;

	t = PINB & 0x0c;												// wm PB2 and PB3
	t |= c & 0xf0;													// wm don't clear PB2 and PB3! 
	PORTB = t; 
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);

	c <<= 4;
	c &= 0xf0;
	c |= PINB & 0x0c;												// wm don't clear PB2 and PB3! 
	PORTB = c;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);
}

void lcdData(char c)
{
	char t;

	t = PINB & 0x0c;												// wm PB2 and PB3
	t |= c & 0xf0;													// wm don't clear PB2 and PB3! 
	t |= (1<<LCD_RS);	
	PORTB = t;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);

	c <<= 4;
	c &= 0xf0;
	c |= PINB & 0x0c;												// wm don't clear PB2 and PB3!	
	c |= (1<<LCD_RS);	
	PORTB = c;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);
}

void lcdCursor(int col, int row)
#ifdef LCD_20x4														// wm
{
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};						// 00, 64, 20, 84
  if ( row >= 4 ) {
	  row = 4-1;		// we count rows starting with 0
  }
  lcdCmd(0x80 | (col + row_offsets[row]));							// SETDDRAMADDR 0x80
  _delay_us(200);
}
#else
{
	if (row==0) lcdCmd(0x80 + col);
	else lcdCmd(0xc0 + col);
	_delay_us(200);
}
#endif

void lcdClear()
{
	// clear display
	lcdCmd(0x01);
	_delay_ms(20);
	
	#ifdef LCD_20x4													// wm
		lcdCmd(0x06);												// Entry mode set
		_delay_ms(1);
	#endif
}

void lcdChar(char c)
{
	lcdData(c);
}

void lcdStr(char *s)
{
	while (*s) 
		lcdData(*s++);
}

