/*
 * Handle the loops in which the program can sit: 
 * vfo, memory, menu and memoryMenu.
 */
#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

void createSmeterChars();
int rxtx();

extern long int freq, lastFreq;
extern int shift;
extern int squelchlevel;
extern int tone;
extern int selectedMemory;
extern int lastSelectedMemory;
extern int tick;
extern int tx;

char str[16];

int Vfo() 
{
	createSmeterChars();

	// get the last freq and settings for mode VFO
	selectedMemory = MAXMEM;
	readMemory(selectedMemory);

	long int savedFreq = freq;
	lastFreq = 0;

	lcdClear();
	lcdCursor(0,0);
	lcdStr("VFO");

	for(;;) {

		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		// handle internal status of trx
		rxtx();

		// handle encoder 
		int c = handleRotary();
		if (c!=0) {
			freq += c * F_RASTER;
			// restart timer for inactivity
			tick = 0;
		}

		// save new vfo frequency in eeprom after ~2 secs inactivity
		if (tick > 200 && freq != savedFreq) {
			writeMemory(selectedMemory);
			savedFreq = freq;
		}

		int push = getRotaryPush();
		if (push) {
			writeMemory(selectedMemory);
			if (push == LONG)
				return MENU;
			else
				return MEMORY;
		}
    }

	return VFO;
}

int Memory()
{
	createSmeterChars();

	selectedMemory = lastSelectedMemory;
	readMemory(selectedMemory);

	lastFreq = 0;

	lcdClear();
	lcdCursor(0,0);
	lcdChar('M');
	lcdChar(0x30 + selectedMemory);

	for(;;) {

		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		// handle internal status of trx
		rxtx();

		// handle encoder 
		int c = handleRotary();
		if (c!=0) {
			if (c>0) {
				if (++selectedMemory > MAXMEM-1)
					selectedMemory = 0;
			}
			else {
				if (--selectedMemory < 0) 
					selectedMemory = MAXMEM-1;
			}

			// show memory channelno.
			lcdCursor(1,0);
			lcdChar(0x30 + selectedMemory);

			// and load it
			readMemory(selectedMemory);
		}

		int push = getRotaryPush();
		if (push) {
			lastSelectedMemory = selectedMemory;
			writeMemory(selectedMemory);
			if (push == LONG)
				return MEMORY_MENU;
			else
				return VFO;
		}
	}

	return VFO;
}

void getSquelch()
{
	lcdCursor(9,1);
	sprintf(str, "  %d", squelchlevel);
	lcdStr(str);
}

void setSquelch()
{
	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> %d ", squelchlevel);
		lcdStr(str);


		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++squelchlevel>64) squelchlevel=64;;
				}
				else {
					if (--squelchlevel<0) squelchlevel = 0;
				}
				break;
			}

			// squelch
			readRSSI();

			if (getRotaryPush())
				return;
		}
	}
}

void getShift()
{
	lcdCursor(9,1);
	sprintf(str, "  %-2d   ", shift);
	lcdStr(str);
}

void setShift()
{
	int sh = shift;

	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> %-2d   ", sh);
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++sh>60) sh = 60;
				}
				else {
					if (--sh<-60) sh = -60;
				}
				break;
			}

			if (getRotaryPush()) {
				shift = sh;
				writeMemory(selectedMemory);
				return;
			}
		}
	}
}

void getCTCSS()
{
	lcdCursor(9,1);
	if (tone>669)
		sprintf(str, "  %d.%d", (int)(tone/10), (int)(tone%10));
	else
		sprintf(str, "  off  ");
	lcdStr(str);
}

void setCTCSS()
{
	int t = tone;

	for (;;) {

		lcdCursor(9,1);
		if (t>669)
			sprintf(str, "> %d.%d", (int)(t/10), (int)(t%10));
		else
			sprintf(str, "> off  ");
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++t>1500) t=1500;
				}
				else {
					if (--t<649) t=649;
				}
				break;
			}
			if (getRotaryPush()) {
				tone = t;
				writeMemory(selectedMemory);
				return;
			}
		}
	}
}

void getMemory()
{
	selectedMemory = lastSelectedMemory;
	lcdCursor(9,1);
	sprintf(str, "  M%d", selectedMemory);
	lcdStr(str);
}

void setMemory(int set)
{

	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> M%d", selectedMemory);
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++selectedMemory > MAXMEM-1)
						selectedMemory = 0;
				}
				else {
					if (--selectedMemory < 0) 
						selectedMemory = MAXMEM-1;
				}
				break;
			}
			if (getRotaryPush()) {
				writeMemory(selectedMemory);
				return ;
			}
		}
	}
}

void scanMemory()
{
	lcdClear();

	for (;;) {
		for (selectedMemory=0; selectedMemory<MAXMEM; selectedMemory++) {
			lcdCursor(0,0);
			lcdStr("M   Scanning");
			lcdCursor(1,0);
			lcdChar('0' + selectedMemory);

			readMemory(selectedMemory);
			setFrequency(freq - IF);
			_delay_ms(200);

			if (tx || getRotaryPush()) return;

			int s = readRSSI();
			if (s > squelchlevel) {
				for (;;) {
					s = rxtx(); 
					if (s > squelchlevel) tick = 0;
					int push = getRotaryPush();
					if (tx || push) {
						if (push == SHORT) break;
						else return;
					}
					if (tick > 200)	break;
				}
			}
		}
	}
}

struct MenuStruct {
	char *name;
	void (* get)();
	void (* set)();
};

#define MAXMENU 3
struct MenuStruct mainMenu[] = {
	{ "Squelch ", &getSquelch, &setSquelch},
	{ "Shift   ", &getShift, &setShift},
	{ "CTCSS   ", &getCTCSS, &setCTCSS},
	{ "Store   ", &getMemory, &setMemory},
//	{ "Spectrum scan ", 0, &Spectrum},
};

#define MAXMEMMENU 3
struct MenuStruct memoryMenu[] = {
	{ "Squelch ", &getSquelch, &setSquelch},
	{ "Shift   ", &getShift, &setShift},
	{ "CTCSS   ", &getCTCSS, &setCTCSS},
	{ "Memory scan   ", 0, &scanMemory},
};

int Menu()
{
	struct MenuStruct *menu = mainMenu;
	int i=0;

	for (;;) {

		lcdCursor(0,1);
		lcdStr("                ");
		lcdCursor(0,1);
		lcdStr(menu[i].name);

		if (menu[i].get) menu[i].get();
	
		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++i>MAXMENU) i=0;
				}
				else {
					if (--i<0) i=MAXMENU;
				}
				break;
			}

			int push = getRotaryPush();
			if (push) {
				if (push == SHORT) {
					menu[i].set();
					// when memory stored, activate that memory
					if (i==3)
						return MEMORY;
				}
				return VFO;
			}
		}
	}

	return VFO;
}

int MemoryMenu()
{
	struct MenuStruct *menu = memoryMenu;

	int i=0;

	for (;;) {	
		lcdCursor(0,1);
		lcdStr("                ");
		lcdCursor(0,1);
		lcdStr(menu[i].name);
		if (menu[i].get) menu[i].get();
	
		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++i>MAXMEMMENU) i=0;
				}
				else {
					if (--i<0) i=MAXMEMMENU;
				}
				break;
			}

			int push = getRotaryPush();
			if (push) {
				if (push == SHORT)
					menu[i].set();
				return MEMORY;
			}
		}
	}

	return MEMORY;
}

