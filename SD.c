#include "SD.h"
#include <avr/io.h>

/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_BLOCK			0x08	/* Block addressing */

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

#define FLAG_TIMEOUT        0x80
#define FLAG_PARAM_ERR      0x40
#define FLAG_ADDR_ERR       0x20
#define FLAG_ERZ_SEQ_ERR    0x10
#define FLAG_CMD_CRC_ERR    0x08
#define FLAG_ILLEGAL_CMD    0x04
#define FLAG_ERZ_RST        0x02
#define FLAG_IN_IDLE_MODE   0x01

#define SELECT()	PORTB &= ~SD_PIN_CS	/* CS = L */
#define	DESELECT()	PORTB |=  SD_PIN_CS	/* CS = H */

static void sdSpiDelay(){

	volatile UInt8 t = 0;
	
	t++;
	t--;
}

static void sdClockSpeed(Boolean speed){
	
	if(speed){
		// Set high speed.
    	SPCR = (1 << SPE) | (1 << MSTR);
    	SPSR = 0;
	}

	else{
		// Set slow speed for initialization.
    	SPCR = (1 << SPE) | (1 << MSTR) | 3;
    	SPSR = 0;
	}
	
}

// From http://www.rjhcoding.com/avrc-sd-interface-1.php

UInt8 sdSpiByte(UInt8 v){
	// load data into register
    SPDR = v;

    // Wait for transmission complete
    while(!(SPSR & (1 << SPIF)));

    // return SPDR
    return SPDR;
}

UInt8 sdCrc7(UInt8* chr,UInt8 cnt,UInt8 crc){

	for(UInt8 a = 0; a < cnt; a++){
		
		UInt8 Data = chr[a];
		
		for(UInt8 i = 0; i < 8; i++){
			
			crc <<= 1;

			if((Data & 0x80) ^ (crc & 0x80)) crc ^= 0x09;
			
			Data <<= 1;
		}
	}
	
	return crc & 0x7F;
}

static inline void sdPrvSendCmd(UInt8 cmd, UInt32 param, Boolean crc){

	/* Select the card */
	DESELECT();
	sdSpiByte(0xFF);
	SELECT();
	sdSpiByte(0xFF);
	
	UInt8 send[6];
	
	send[0] = cmd | 0x40;
	send[1] = param >> 24;
	send[2] = param >> 16;
	send[3] = param >> 8;
	send[4] = param;
	send[5] = crc ? (sdCrc7(send, 5, 0) << 1) | 1 : 0;
	
	for(cmd = 0; cmd < crc ? 6 : 5; cmd++) sdSpiByte(send[cmd]);
}

static UInt8 sdPrvSimpleCommand(UInt8 cmd, UInt32 param, Boolean crc){  //do a command, return R1 reply
	
	UInt8 ret;
	UInt8 i = 0;
	
	sdPrvSendCmd(cmd, param, crc);
	
	do{     //our max wait time is 128 byte clocks (1024 clock ticks)
		
		ret = sdSpiByte(0xFF);
		
	}while(i++ < 128 && (ret == 0xFF));
	
	return ret;
}

static UInt8 sdPrvReadData(UInt8* data, UInt16 sz){
	
	UInt8 ret;
	UInt8 tries = 200;
	
	do{
		ret = sdSpiByte(0xFF);
		if((ret & 0xF0) == 0x00) return ret;    //fail
		if(ret == 0xFE) break;
	}while(tries--);
	
	if(!tries) return 0xFF;
	
	*data = ret;
	
	while(sz--) *data++ = sdSpiByte(0xFF);
	
	return 0;
}

UInt32 sdPrvGetBits(UInt8* data,UInt32 numBytesInArray,UInt32 startBit,UInt32 len){//for CID and CSD data..
	
	UInt32 bitWrite = 0;
	UInt32 ret = 0;
	
	do{
		
		UInt32 bit;
		UInt32 byte = (bit = numBytesInArray*8 - startBit - 1) / 8;

		bit = 7 - (bit % 8);
		
		ret |= ((data[byte] >> bit) & 1) << (bitWrite++);
		
		startBit++;
	}while(--len);
	
	return ret;
}

static UInt32 sdPrvGetCardNumBlocks(Boolean mmc,UInt8* csd){
	
	UInt32 ver = sdPrvGetBits(csd,16,126,2);
	UInt32 cardSz = 0;
	
	if(ver == 0 || (mmc && ver <= 2)){

		UInt32 divTimes = 9;        //from bytes to blocks division
		
		UInt32 blockLen = 1UL << sdPrvGetBits(csd,16,80,4);
		UInt32 blockNr = (sdPrvGetBits(csd,16,62,12) + 1) * (1UL << (sdPrvGetBits(csd,16,47,3) + 2));
		
		/*
			 multiplying those two produces result in bytes, we need it in blocks
			 so we shift right 9 times. doing it after multiplication might fuck up
			 the 4GB card, so we do it before, but to avoid killing significant bits
			 we only cut the zero-valued bits, if at the end we end up with non-zero
			 "divTimes", divide after multiplication, and thus underuse the card a bit.
			 This will never happen in reality since 512 is 2^9, and we are
			 multiplying two numbers whose product is a multiple of 2^9, so they
			 togethr should have at least 9 lower zero bits.
		*/
		
		while(divTimes && !(blockLen & 1)){
			
			blockLen >>= 1;
			divTimes--;
		}
		while(divTimes && !(blockNr & 1)){
			
			blockNr >>= 1;
			divTimes--;
		}
		
		cardSz = (blockNr * blockLen) >> divTimes;
	}
	else if(ver == 1){
		
		cardSz = sdPrvGetBits(csd,16,48,22)/*num 512K blocks*/ << 10;
	}
	
	
	return cardSz;
}

// From https://github.com/greiman/PetitFS/blob/master/src/avr_mmcp.cpp

Boolean sdInit(SD* sd){

	UInt8 cmd, n, ty = 0, ocr[4], respBuf[16];
	unsigned long tmr;

	sd->inited = false;
	sd->SD = false;

	sdClockSpeed(false);

	DESELECT();
	for (n = 10; n; n--) sdSpiByte(0xFF);	/* 80 dummy clocks with CS=H */

	if (sdPrvSimpleCommand(CMD0, 0, true) == 1) {			/* GO_IDLE_STATE */
		if (sdPrvSimpleCommand(CMD8, 0x1AA, true) == 1) {	/* SDv2 */
			for (n = 0; n < 4; n++) ocr[n] = sdSpiByte(0xFF);		/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {			/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 10000; tmr && sdPrvSimpleCommand(ACMD41, 1UL << 30, true); tmr--) sdSpiDelay();	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (tmr && sdPrvSimpleCommand(CMD58, 0, true) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = sdSpiByte(0xFF);
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 (HC or SC) */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (sdPrvSimpleCommand(ACMD41, 0, true) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 10000; tmr && sdPrvSimpleCommand(cmd, 0, true); tmr--) sdSpiDelay();	/* Wait for leaving idle state */
			if (!tmr || sdPrvSimpleCommand(CMD16, 512, true) != 0)			/* Set R/W block length to 512 */
				ty = 0;
		}
	}

	if(sdPrvSimpleCommand(59, 0, true) & FLAG_TIMEOUT) return false; //crc off
	if(sdPrvSimpleCommand(9, 0, false) & FLAG_TIMEOUT) return false; //read CSD

	DESELECT();
	sdSpiByte(0xFF);

	if(sdPrvReadData(respBuf, 16)) return false;

	sd->numSec = sdPrvGetCardNumBlocks(!sd->SD, respBuf);
	sd->inited = !ty;

	sdClockSpeed(true);

	return sd->inited;
}

Boolean sdSecRead(SD* sd, UInt32 sec, void* buf, UInt16 sz){   //CMD17

	UInt8 retry = 0;

	//PIND = (UInt8)(1 << 2);   //LED_r

	if(!sd->inited) return false;

	do{

		if(sdPrvSimpleCommand(17, sd->HC ? sec : sec << 9, false) & FLAG_TIMEOUT) return false;

		if(!sdPrvReadData(buf, sz)){ // Read sz bytes to buf
			return true;
			break;
		} 

	}while(++retry < 5);    //retry up to 5 times

	//PIND = (UInt8)(1 << 2);   //LED_r

	DESELECT();
	sdSpiByte(0xFF);
}


Boolean sdSecWrite(SD* sd, UInt32 sec, UInt8* buf, UInt16 sz){  //CMD24

	UInt8 retry = 0, v;
	
	//writechar('W');

	//PIND = (UInt8)(1 << 3);   //LED_w

	if(!sd->inited) return false;

	do{
		
		if(sdPrvSimpleCommand(24, sd->HC ? sec : sec << 9, false) & FLAG_TIMEOUT) return false;
	
		sdSpiByte(0xFF);    //as per SD-spi spec, we give it 8 clocks to consider the ramifications of the command we just sent
		sdSpiByte(0xFF);    //start of data block

		for(UInt16 v16 = 0; v16 < sz; v16++) sdSpiByte(*buf++);    //data
		
		while((v = sdSpiByte(0xFF)) == 0xFF);   //wait while card isnt answering
		while(sdSpiByte(0xFF) != 0xFF); //wait while card is busy
	
		if((v & 0x1F) == 5){
			return true;
		} 
	
	}while(++retry < 5);        //retry up to 5 times

	//PIND = (UInt8)(1 << 3);   //LED_w

	DESELECT();
	rcv_spi();
}

/* @raspiduino's code */

void ramRead(SD* sd, UInt32 addr, UInt8* buf, UInt8 sz){

	UInt8* b;
	UInt8 sec_addr = addr%SD_BLOCK_SIZE;

	sdSecRead(sd, sd->numSec + (addr - 1024*16)/SD_BLOCK_SIZE, b, sz + sec_addr); // Read the sector contain the addr

	for(UInt8 i = 0 ; i < sz; i++){
		buf[i] = b[i + sec_addr];
	}
}

void ramWrite(SD* sd, UInt32 addr, UInt8* buf, UInt8 sz){

	UInt8* b;
	UInt8 sec_addr = addr%SD_BLOCK_SIZE;
	UInt32 sec = sd->numSec + (addr - 1024*16)/SD_BLOCK_SIZE; // Read the data from the begining of that sector to the addr

	sdSecRead(sd, sec, b, addr%SD_BLOCK_SIZE);

	for(UInt8 i = 0; i < sz; i++){
		b[i+sec_addr] = buf[i];
	}

	sdSecWrite(sd, sec, b, addr%SD_BLOCK_SIZE + sz); // Write it
}
