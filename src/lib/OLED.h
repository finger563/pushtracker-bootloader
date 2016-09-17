#ifndef OLED_H
#define OLED_H

#define SSD1339
//#define SSD1351		// for newer screens with 1351 gdd

#ifdef SSD1339
// data bus for LCD, pins on port 0
#define D0 16
#define D1 17
#define D2 18
#define D3 19
#define D4 20
#define D5 21
#define D6 22
#define D7 23
#define D8 24

// OLED data port
#define LCD_DATA ((1<<D0)|(1<<D1)|(1<<D2)|(1<<D3)|(1<<D4)|(1<<D5)|(1<<D6)|(1<<D7)|(1<<D8))

// other OLED pins
#define LCD_RD    (1<<25)			// P1.25
#define LCD_RW    (1<<26)			// P1.26
#define LCD_CS    (1<<27)			// P1.27
#define LCD_DC    (1<<28)			// P1.28
#define LCD_RSTB  (1<<29)			// P1.29

#define BS1			(1<<30)			// P1.30
#define BS2			(1<<31)			// P1.31

//==========================================
//===========If Using new OLED==============
//==========================================
#else
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
#endif

#define OLED_END 	(unsigned int)127

// inialize OLED
void OLED_init(void);
void OLED_ShutDown(void);
void OLED_TurnOn(void);
void ClearScreen(void);
void ClearWindow(unsigned char x1,unsigned char y1, unsigned char x2, unsigned char y2);

// reset Controller
void Reset_SSD1339(void);

// write command or data
void write_c(unsigned char out_command);
void write_d(unsigned char out_data);
void write_color(unsigned short out_color);

void BufferToScreen(unsigned char x_start,unsigned char y_start,unsigned char x_end, unsigned char y_end);

#endif
