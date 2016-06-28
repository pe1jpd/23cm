/*
 * All code to store and load settings to EEPROM.
 *
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

#define MAGICNUMBER 18296

extern int squelchlevel;
extern int mode;

extern long int freq;
extern int shift;
extern int tone;
extern long int toneCount;
extern int lastSelectedMemory;

// write default values in memories
void clearMemory()
{
	squelchlevel = 0;
	mode = VFO;
	lastSelectedMemory = 0;

	writeGlobalSettings();

	freq = 1298000UL;
	tone = 669;
	shift = 0;

	for (int mem=0; mem<=MAXMEM; mem++) {
		writeMemory(mem);
	}
}

// Read global settings
void readGlobalSettings()
{
	unsigned int address = 0;

	int magic = eeprom_read_word((unsigned int *)address);
	address += 2;
	if (magic != MAGICNUMBER)
		clearMemory();

	squelchlevel = eeprom_read_word((unsigned int *)address);
	address += 2;
	mode = eeprom_read_word((unsigned int *)address);
	address += 2;
	lastSelectedMemory = eeprom_read_word((unsigned int *)address);
	address += 2;
}

void writeGlobalSettings()
{
	unsigned int address = 0;

	eeprom_write_word((unsigned int *)address, (int)MAGICNUMBER);
	address += 2;
	eeprom_write_word((unsigned int *)address, (int)squelchlevel);
	address += 2;
	eeprom_write_word((unsigned int *)address, (int)mode);
	address += 2;
	eeprom_write_word((unsigned int *)address, (int)lastSelectedMemory);
	address += 2;
}

// Read memory settings from EEPROM
void readMemory(int memory)
{
	unsigned int address = 0;

	if (memory<0 || memory>MAXMEM) {
		return;
	}
  
	address = 100 + memory*10;

	freq = eeprom_read_dword((unsigned long int *)address);
	address += 4;
	tone = eeprom_read_word((unsigned int *)address);
	address += 2;
	shift = eeprom_read_word((unsigned int *)address);
	address += 2;

	// set countervalue for ISR
	toneCount = 5*F_CPU/tone;
}


// Write memory settings to EEPROM
void writeMemory(int memory)
{
	if (memory<0 || memory>MAXMEM) {
  		return;
	}

	unsigned int address = 100 + memory*10;

	eeprom_write_dword((unsigned long int *)address, (long int)freq);
	address += 4;
	eeprom_write_word((unsigned int *)address, (int)tone);
	address += 2;
	eeprom_write_word((unsigned int *)address, shift);
	address += 2;
}


