/******************************************************************************
 *
 * $RCSfile$
 * $Revision: 124 $
 *
 * This module provides the interface definitions for setting up and
 * controlling the various interrupt modes present on the ARM processor.
 * Copyright 2004, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 *
 *****************************************************************************/
#ifndef OLED_H
#define OLED_H

#define SSD1339
//#define SSD1351		// for newer screens with 1351 gdd

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

#define OLED_PWR	(1<<30)			// P0.30

#define OLED_ON		FIO0SET = OLED_PWR
#define OLED_OFF	FIO0CLR	= OLED_PWR

#define OLED_END 	(unsigned char)127
#define GRAPH_START	(unsigned char)45
#define GRAPH_END	(unsigned char)115
#define GRAPH_RIGHT	(unsigned char)127
#define GRAPH_LEFT	(unsigned char)0
#define TIME_HEIGHT	(unsigned char)120
#define TEXT_OFFSET	(unsigned char)5
#define TEXT_SPACING	(unsigned char)4
#define VAL_OFFSET	(unsigned char)90
#define TEXT_HEIGHT	(unsigned char)43


// inialize OLED
void OLED_init(void);
void ClearScreen(void);

// write command or data
void write_c(unsigned char out_command);
void write_d(unsigned char out_data);
unsigned char read_d(void);

void ScreenToBuffer(unsigned char x_start,unsigned char y_start,unsigned char x_end, unsigned char y_end);
void BufferToScreen(unsigned char x_start,unsigned char y_start,unsigned char x_end, unsigned char y_end);


#endif
