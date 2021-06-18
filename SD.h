#ifndef _SD_H_
#define _SD_H_


#include "types.h"


typedef struct{
	
	UInt32 numSec;
	UInt8 HC	: 1;
	UInt8 inited	: 1;
	UInt8 SD	: 1;
	
}SD;

#define SD_BLOCK_SIZE		512

/*
	Since atmega328p doesn't have enough pin for a real RAM, we
	need to use virtual RAM on SD card.

	The Linux Operating System will be saved from the first sector
	on the SD card to DISK_IMAGE_SIZE/SD_BLOCK_SIZE sector.

	If you use the provided image, it's from sector 0 to sector
	1052704.

	From SoC.c: #define RAM_SIZE	0x01000000UL	//16M @ 0xA0000000
	So the virtual RAM size will be 16MB

	The virtual RAM on the SD card will be store at the last sector
	of the SD minus 32 sector.

	The number of sector on the card can be returned by sdGetNumSec
	function in SD.c, so the RAM start at sector sdGetNumSec - 33.
*/

// SD card pin
#define SD_PIN_MOSI     (1 << PINB3) // Arduino UNO's digital pin 11
#define SD_PIN_MISO     (1 << PINB4) // Arduino UNO's digital pin 12
#define SD_PIN_SCLK     (1 << PINB5) // Arduino UNO's digital pin 13

Boolean sdInit(SD* sd);
UInt32 sdGetNumSec(SD* sd);
Boolean sdSecRead(SD* sd, UInt32 sec, void* buf, UInt16 sz);
Boolean sdSecWrite(SD* sd, UInt32 sec, UInt8* buf, UInt16 sz);
void ramRead(SD* sd, UInt32 addr, UInt8* buf, UInt8 sz);
void ramWrite(SD* sd, UInt32 addr, UInt8* buf, UInt8 sz);

#endif
