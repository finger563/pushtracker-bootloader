/*
  SparkFun's ARM Bootloader
  7/28/08
	
  Bootload binary file(FW.SFE) over USB

*/	
#include "LPC214x.h"

//UART0 Debugging
#include "OLED.h"
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

struct fat16_file_struct* handle;

void load_data(void);

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
  // only gets here if firmware run fails!
  FIO0DIR |= (1<<21);
  FIO0CLR |= (1<<21);
  int state = 0;
  while (1) {
    state = !state;
    if (state)
      FIO0SET |= (1<<21);
    else
      FIO0CLR |= (1<<21);
    delay_ms(500);
  }
}
