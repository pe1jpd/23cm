#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

// internal function prototypes
void lcdCmd(char c);
void lcdData(char c);

void lcdInit()
{
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

}

void lcdCmd(char c)
{
	char t;

	t = c & 0xf0;
	PORTB = t;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);

	c <<= 4;
	c &= 0xf0;
	PORTB = c;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);
}

void lcdData(char c)
{
	char t;

	t = c & 0xf0;	
	t |= (1<<LCD_RS);	
	PORTB = t;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);

	c <<= 4;
	c &= 0xf0;
	c |= (1<<LCD_RS);	
	PORTB = c;
	sbi(PORTB, LCD_E);
	_delay_us(2);
	cbi(PORTB, LCD_E);
	_delay_us(200);
}

void lcdCursor(int col, int row)
{
	if (row==0) lcdCmd(0x80 + col);
	else lcdCmd(0xc0 + col);
	_delay_us(200);
}

void lcdClear()
{
	// clear display
	lcdCmd(0x01);
	_delay_ms(20);

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

