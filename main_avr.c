#define F_CPU	16000000UL
#define BAUD	9600UL
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <avr/boot.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#undef SPSR	//avr code defines it

#include "SoC.h"
#include "SD.h"
#include "callout_RAM.h"

SD sd;

static int readchar(){
	if(UCSR0A & (1<<RXC0)){
		return UDR0;
	}
	else return CHAR_NONE;
}

void writechar(int chr){
	while(!(UCSR0A & (1<<UDRE0)));	//busy loop
	UDR0 = chr;
}

int rootOps(void* userData, UInt32 sector, void* buf, UInt8 op){
	
	SD* sd = userData;
	
	switch(op){
		case BLK_OP_SIZE:
			
			if(sector == 0){	//num blocks
				*(unsigned long*)buf = sdGetNumSec(sd);
			}
			else if(sector == 1){	//block size
				
				*(unsigned long*)buf = SD_BLOCK_SIZE;
			}
			else return 0;
		
		case BLK_OP_READ:
			
			return sdSecRead(sd, sector, buf, SD_BLOCK_SIZE);
			
		
		case BLK_OP_WRITE:
			
			return sdSecWrite(sd, sector, buf, SD_BLOCK_SIZE);
	}
	return 0;	
}

void init(){
	
	cli();

	//wdt
	{		
		asm("cli");
		wdt_reset();
		wdt_disable();
	}

	// Init the UART of Arduino UNO
    // From http://www.rjhcoding.com/avrc-uart.php
    // ubrr is calculated by ubrr = (f/(16baud))-1

    // set baudrate in UBRR
    UBRR0L = (uint8_t)(103);
    UBRR0H = (uint8_t)(103 >> 8);

    // Set Frame Format
    UCSR0C = (0 << UMSEL00) | (0 << UPM00) | (0 << USBS0) | (3<<UCSZ00);

    // enable the transmitter and receiver
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    // From http://www.rjhcoding.com/avrc-sd-interface-1.php

    // Set up hardware SPI for SD card IO
    DDRB |= SD_PIN_MOSI | SD_PIN_SCLK;
}

Boolean coRamAccess(_UNUSED_ CalloutRam* ram, UInt32 addr, UInt8 size, Boolean write, void* bufP){

	UInt8* b = bufP;
	
	if(write) ramWrite(&sd, addr, b, size);
	else ramRead(&sd, addr, b, size);

	return true;
}

static SoC soc;

int main(){

	init();

	err_str("uARM - an ARM emulator running on atmega328p!");
	
	if(!sdInit(&sd)) err_str("sd init failed");

	err_str("SD init completed!"); // For debuging only

	socInit(&soc, socRamModeCallout, coRamAccess, readchar, writechar, rootOps, &sd);
	
	if(!(PIND & 0x10)){	//hack for faster boot in case we know all variables & button is pressed
		UInt32 s = 786464UL;
		UInt32 d = 0xA0E00000;
		UInt8* b = (UInt8*)soc.blkDevBuf;

		for(UInt32 i = 0; i < 4096; i++){
			sdSecRead(&sd, s++, b, SD_BLOCK_SIZE);
			for(UInt16 j = 0; j < 512; j += 32, d+= 32){
				
				ramWrite(&sd, d, b + j, 32);
			}
		}
		soc.cpu.regs[15] = 0xA0E00512UL;
	}

	socRun(&soc);

	while(1); // Emulation stop for some reason
}

void err_str(const char* str){
	
	char c;
	
	while((c = *str++) != 0) writechar(c);
}

void* emu_alloc(_UNUSED_ UInt32 size){
	
	//err_str("No allocations in avr mode please!");
	
	return 0;
}