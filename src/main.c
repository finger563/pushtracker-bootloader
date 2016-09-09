/*
	SparkFun's ARM Bootloader
	7/28/08
	
	Bootload binary file(FW.SFE) over USB

*/	
#include "LPC214x.h"

//UART0 Debugging
//#include "OLED.h"
#include "serial.h"
#include "rprintf.h"

//Memory manipulation and basic system stuff
#include "firmware.h"
#include "system.h"

//SD Logging
#include "rootdir.h"
#include "sd_raw.h"
#include "System/fat16.h"

//USB
#include "main_msc.h"

//This is the file name that the bootloader will scan for
#define FW_FILE "FW.SFE"

// data bus for LCD, pins on port 0
#define D0 16
#define D1 17
#define D2 18
#define D3 19
#define D4 20
#define D5 21
#define D6 22
#define D7 23

// OLED data port
#define LCD_DATA ((1<<D0)|(1<<D1)|(1<<D2)|(1<<D3)|(1<<D4)|(1<<D5)|(1<<D6)|(1<<D7))

// other OLED pins
#define LCD_RD    (1<<25)			// P1.25
#define LCD_RW    (1<<26)			// P1.26
#define LCD_CS    (1<<27)			// P1.27
#define LCD_DC    (1<<28)			// P1.28
#define LCD_RSTB  (1<<29)			// P1.29

#define BS0			(1<<30)			// P1.30
#define BS1			(1<<31)			// P1.31

#define OLED_PWR	(1<<30)			// P0.30

#define OLED_ON		IOSET0 = OLED_PWR
#define OLED_OFF	IOCLR0 = OLED_PWR

/* OLED Commands */
enum
{	
	SSD1351ColAddr = 0x15,	// 2 data bytes: start, end addr (range=0-131; def=0,131)
	
	SSD1351WriteRAM = 0x5c,	// enable MCU write to RAM
	SSD1351ReadRAM = 0x5d,	// enable MCU read from RAM
	
	SSD1351RowAddr = 0x75,	// 2 data bytes: start, end addr (range=0-131; def=0,131)
	
	//SSD1351DrawSettings = 0x92,
	SSD1351HScroll = 0x96,
	SSD1351StartScroll = 0x9f,
	SSD1351StopScroll = 0x9e,

	SSD1351Settings = 0xa0,	// 1 data byte (see doc) [bit3=reserved] [bit7:6=01 for 65k color (def't)]	
	SSD1351StartLine = 0xa1,	// 1 data byte: vertical scroll by RAM (range=0-131; def=0)
			// ^ LOCKED
	SSD1351Offset = 0xa2,	// 1 data byte: vertical scroll by Row (range=0-131; def=0)
	SSD1351DispAllOff = 0xa4,
	SSD1351DispAllOn = 0xa5,		// all pixels have GS15
	SSD1351DispNormal = 0xa6,	// default
	SSD1351DispInverse = 0xa7,	// GS0->GS63, GS1->GS62, ...
	SSD1351Config = 0xAB,	// 1 data byte (see doc) 
		/* bit 0: 0 = ext Vcc; 1 = DC-DC converter for Vcc
		   bit [7:6]: 0 = 8 bit interface (reset)
						1 = 16 bit 
						3 = 18 bit*/
	SSD1351NOP = 0xAD,
	SSD1351SleepOn = 0xae,		// (display off)
	SSD1351SleepOff = 0xaf,		// (display on)

	SSD1351SetPeriod = 0xb1,	// 1 data byte: low nibble: phase 1 (reset) period [4]; high nibble: phase 2 [7](pre-charge) period; 1-16DCLK
			// ^ LOCKED
	SSD1351SetPerformance = 0xb2,	// 3 data bytes 0,0,0 = normal (def't), A4h,0,0=enhance performance
	SSD1351Clock = 0xb3,		// 1 data byte: low nibble: divide by DIVSET+1 [0]; high nibble: osc freq [9? d?]
			// ^ LOCKED
	SSD1351VSL = 0xB4,			// 3 data bytes:  A = 0b101000XX,  A1:A0 => 00: External VSL, 10: Internal VSL; B = 0b10110101; C = 0b01010101
	
	SSD1351GPIO = 0xB5, 		// A1:0 => GPIO0, A3:2 => GPIO1
	SSD1351SecondPeriod = 0xB6,	// A3:A0 => 0:invalid, 1: 1 DCLKS, 2: 2 DCLKS, 3: 3 Dclks,...15: 15 DCLKS
	SSD1351GrayPulseWidth = 0xb8,	// 63 bytes (see doc)
	SSD1351LinearLUT = 0xb9,		// reset to default (linear grayscale) Look Up Table (PW1=1, PW2=3, PW3=GRAPH_LEFT ... PW63=125)
	SSD1351PrechargeV = 0xbb,	// 1 byte: 0: 0.2*Vref, 1f: 0.6*Vcc, high 3 bits = 0  
				// ^ LOCKED
	SSD1351SetVcomh = 0xbe,		// 1 byte: 0: 0.72*Vref, 1f: 0.84*Vref (def't)  A2:0,   rest are 0
	
	SSD1351ColorContrast = 0xc1,	// 3 bytes: colors A, B, C; def't=0x80 (sets current)
	SSD1351Contrast = 0xc7,		// 1 byte: reduce output current (all colors) to (low nibble+1)/16 [def't=0f]
	SSD1351MUXRatio = 0xca,	// 1 byte: 16MUX-128MUX (range=15-OLED_END; def=OLED_END)
	
	SSD1351Lock = 0xfd,		// 1 byte: 0x12=unlock (def't); 0x16=lock
};

struct fat16_file_struct* handle;

const int dxScreen = 128;	// number of columns on screen
const int dyScreen = 128;	// number of rows on screen
const int dxRAM = 132;		// number of cols stored in RAM (including off-screen area)
const int dyRAM = 132;		// number of rows stored in RAM (including off-screen area)

// 'last' values are largest values that are visible on the screen 
const int xLast = 127;
const int yLast = 127;
// 'most' values are largest valid values (e.g. for drawing) 
const int xMost = 131;
const int yMost = 131;


void load_data(void);
void OLED_init(void);
void write_c(unsigned char out_command);
void write_d(unsigned char out_data);

void load_data(void)
{
	#define READBUFSIZE 1024

	sd_raw_init();
	openroot();

	/* readbuf MUST be on a word boundary */
	unsigned char readbuf[READBUFSIZE];
	char filename[12] = "splash.bin";

    unsigned int read;
    unsigned int i;

	/* Open the file */
	if(root_file_exists(filename))
	{
		handle = root_open(filename);
		
		/* Clear the buffer */
		for(i=0;i<READBUFSIZE;i++)
		{
			readbuf[i]=0;
		}
		
		write_c(0x15);	// set column start and end addresses
		write_d(0);
		write_d(127);
		write_c(0x75);	// set row start and end adresses
		write_d(0);
		write_d(127);
		write_c(0x5C);	// write to RAM command
		
		/* Read the file contents, and copy them to the display buffer */
		while( (read=fat16_read_file(handle,(unsigned char*)readbuf,READBUFSIZE)) > 0 )
		{
			for(i=0;i<read;i++)
			{
				//disp_buff[row][col] = readbuf[i];
				write_d(readbuf[i]);
			}
		}
	}
	else 
	{
		handle = root_open_new(filename);
	}
    /* Close the file! */
    sd_raw_sync();
    fat16_close_file(handle);
}

void OLED_init(void)
{	
	SCS = 2;
	unsigned long _dummy;
	_dummy = PINSEL2;
	_dummy &= ~((1<<2)|(1<<3));
	PINSEL2 = _dummy;	
	
	FIO1DIR |= LCD_DATA|LCD_DC|LCD_RW|LCD_CS|LCD_RD|LCD_RSTB|BS0|BS1;
	IODIR0 |= OLED_PWR;
	
	FIO1SET = BS1|BS0;
	
	OLED_OFF;
	delay_ms(500);
	
	FIO1CLR = LCD_RD|LCD_CS|LCD_RW;

	delay_ms(20);
	FIO1CLR = LCD_RSTB;
	delay_ms(200);
	FIO1SET = LCD_RSTB;
	delay_ms(20);
	write_c(SSD1351Lock);
	write_d(0x12);
	write_c(SSD1351Lock);
	write_d(0xB1);
	write_c(SSD1351SleepOn);
	write_c(SSD1351Clock); // clock & frequency
	write_d(0xF1); // clock=Divser+1 frequency=fh
	write_c(SSD1351MUXRatio); // Duty
	write_d(127); // OLED_END+1
	write_c(SSD1351Settings); // Set Re-map / Color Depth
	write_d(0x34);//0xb4); 34		// 26 turns it upside down
	write_c(SSD1351ColAddr);
	write_d(0);
	write_d(127);
	write_c(SSD1351RowAddr);
	write_d(0);
	write_d(127);
	write_c(SSD1351StartLine); // Set display start line
	write_d(0x00); // 00h start
	write_c(SSD1351Offset); // Set display offset
	write_d(0x00); // 80h start
	write_c(SSD1351GPIO);
	write_d(0x00);
	write_c(SSD1351Config); // Set Master Configuration
	write_d(0x01); // internal VDD
	write_c(SSD1351SetPeriod); // Set pre & dis_charge
	write_d(0x52); 		// 32
	write_c(SSD1351VSL);
	write_d(0xA0);		// A1:A0 => 00: External VSL, 10: Internal VSL
	write_d(0xB5);
	write_d(0x55);
	write_c(SSD1351PrechargeV); // Set pre-charge voltage of color A B C
	write_d(0x17);		//	-- reset = 0x17
	write_c(SSD1351SetVcomh); // Set VcomH
	write_d(0x05); // reset value 0x05
	write_c(SSD1351ColorContrast); // Set contrast current for A B C
	write_d(0xC8); // Color A	-- default
	write_d(0x80); // Color B
	write_d(0xC8); // Color C
	write_c(SSD1351SetPerformance);
	write_d(0xA4);
	write_d(0x00);
	write_d(0x00);
	write_c(SSD1351Contrast); // Set master contrast
	write_d(0x0f); // no change		-- was 0x0f
	write_c(SSD1351SecondPeriod);
	write_d(0x01);
	write_c(SSD1351DispNormal); // Normal display
	OLED_ON;
	delay_ms(100);
	write_c(SSD1351SleepOff); // Display on
}

void write_c(unsigned char out_command)
{	
	FIO1DIR |= LCD_DATA;
	//LCD_out(out_command);
	FIO1PIN2 = out_command;
	//FIO0SET |= LCD_RD;
	FIO1CLR = LCD_DC|LCD_RW;	//	|LCD_CS
	//FIO0SET |= LCD_CS;
	FIO1SET = LCD_RD;
	FIO1CLR = LCD_RD;
	//FIO0SET |= LCD_CS;
}

void write_d(unsigned char out_data)
{
	FIO1DIR |= LCD_DATA;
	//LCD_out(out_data);
	FIO1PIN2 = out_data;
	FIO1SET = LCD_DC;
	FIO1CLR = LCD_RW;		//|LCD_CS
	//FIO0SET |= LCD_CS;
	FIO1SET = LCD_RD;
	FIO1CLR = LCD_RD;
	//FIO0SET |= LCD_CS;
}

int main (void)
{
	boot_up();						//Initialize USB port pins and set up the UART
	OLED_init();
	rprintf("Boot up complete\n");	

	if(IOPIN0 & (1<<23))			//Check to see if the USB cable is plugged in
	{	
		load_data();
		main_msc();					//If so, run the USB device driver.
	}
	else{
		rprintf("No USB Detected\n");
	}
	
	//Init SD
	if(sd_raw_init())				//Initialize the SD card
	{
		openroot();					//Open the root directory on the SD card
		rprintf("Root open\n");
		
		if(root_file_exists(FW_FILE))	//Check to see if the firmware file is residing in the root directory
		{
			rprintf("New firmware found\n");
			load_fw(FW_FILE);			//If we found the firmware file, then program it's contents into memory.
			rprintf("New firmware loaded\n");
		}
	}
	else{
		//Didn't find a card to initialize
		rprintf("No SD Card Detected\n");
		delay_ms(250);
	}
	rprintf("Boot Done. Calling firmware...\n");
	delay_ms(500);
	call_firmware();					//Run the new code!

	while(1);
}
