#define F_CPU 		1000000UL

#define BOARD2
#define ADF4153
#define version		"4.0"

#define F_REF		13000		// in kHz
#define	IF			69300UL		// in kHz

#define sbi(x,y)	x |= _BV(y) //set bit - using bitwise OR operator 
#define cbi(x,y)	x &= ~(_BV(y)) //clear bit - using bitwise AND operator
#define tbi(x,y)	x ^= _BV(y) //toggle bit - using bitwise XOR operator
#define is_high(x,y) (x & _BV(y) == _BV(y)) //check if the y'th bit of register 'x' is high ... test if its AND with 1 is 1

#define max(x,y)	(x>y ? x : y)
#define min(x,y)	(x>y ? y : x)

#define FALSE 		0
#define TRUE		!FALSE
#define SHORT		1
#define	LONG		2

// various
#ifdef BOARD2
#define Smeter		PC5
#define MUTE		PC4
#define TXON		PC3
#define LE	     	PC2
#define DATA    	PC1
#define CLK     	PC0
#endif

#ifdef BOARD1
#define Smeter		PC1
#define MUTE		PC0
#define TXON		PC2
#define LE	     	PC5
#define DATA    	PC4
#define CLK     	PC3
#endif

// rotary & switches
#define PTT			PD0
#define REVERSE		PD1
#define PUSH		PD2
#define ROTINT		PD3
#define	ROT			PD4
#define	DN			PD5
#define	UP			PD6

// LCD
#define	LCD_D7		PB7
#define	LCD_D6		PB6
#define	LCD_D5		PB5
#define	LCD_D4		PB4
#define	LCD_RS		PB1
#define	LCD_E		PB0

#define S			PB2
#define Beep		PB3

// modes, SPECTRUM is last mode
#define VFO			1
#define MENU		2
#define MEMORY		3
#define MEMORY_MENU	4
#define SPECTRUM	5

#define MAXMEM 		10			// 9 memories and 1 vfo

// function prototypes
void initPLL();
void setPLL(unsigned long r);

void setFrequency(unsigned long freq);
void displayFrequency(long int f);

void lcdInit();
void lcdCmd(char c);
void lcdData(char c);
void lcdCursor();
void lcdClear();
void lcdChar(char c);
void lcdStr(char *s);

int readRSSI();
void displaySmeter();

int getRotaryPush();
int handleRotary();
void readMemory();
void writeMemory();
void readGlobalSettings();
void writeGlobalSettings();
int Vfo();
int Spectrum();
int Memory();
int Menu();
int MemoryMenu();
int readUpDn();
