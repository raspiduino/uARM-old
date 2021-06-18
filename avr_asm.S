.globl gSdSpiFast
.globl sdSpiByteSlow
.globl sdSpiByte
sdSpiByte:
	
	lds     r20, gSdSpiFast
	tst     r20 		  ; See if spi fast or slow mode
	brne	sdSpiByteFast ; If fast mode jump to assembly code
	rjmp    sdSpiByteSlow ; If not, jump to C code

sdSpiByteFast:
	
	in   r25, 0x05	;	t1 = PORTB
	andi r25, 0x28	;	t1 &=~ (SD_PIN_MOSI | SD_PIN_SCLK)
	ldi  r23, 0x08	;	t2 = SD_PIN_MOSI
	ldi  r22, 0x20	;	t3 = SD_PIN_SCLK
	ldi  r21, 8	;	i = 8
spi_loop:		; loop:
	out  0x05, r25	;	PORTB = t1
	sbrc r24, 7	;	if(v & 0x80)
	out  0x03, r23	;		PINB = t2
	out  0x03, r22	;	PINB = t3
	lsl  r24	;	v <<= 1
	sbic 0x03, 4;	if(PINB & SD_PIN_MISO)
	inc  r24	;		v++
	dec  r21	;	i--
	out  0x03, r22	;	PINB = t3
	brne spi_loop	;	if(i) goto loop
	
	ret
