APP	= uARM
CC	= gcc
LD	= gcc

BUILD ?= debug

ifeq ($(BUILD), avr)

	CC_FLAGS        = -Os -mmcu=atmega328p -I/usr/lib/avr/include -DEMBEDDED -D_SIM -ffunction-sections -DAVR_ASM
	LD_FLAGS        = -Os -mmcu=atmega328p -Wl,--gc-sections
	CC              = avr-gcc
	LD              = avr-gcc
	EXTRA           = avr-size -Ax $(APP) && avr-objcopy -j .text -j .data -O ihex $(APP) $(APP).hex
	EXTRA_OBJS      = SD.o main_avr.o avr_asm.o
endif

ifeq ($(BUILD), debug)
	CC_FLAGS	= -O0 -g -ggdb -ggdb3 -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DLCD_SUPPORT
	LD_FLAGS	= -O0 -g -ggdb -ggdb3 -lSDL
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), profile)
	CC_FLAGS	= -O3 -g -pg -fno-omit-frame-pointer -march=core2 -mpreferred-stack-boundary=4  -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
	LD_FLAGS	= -O3 -g -pg -lSDL
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), opt)
	CC_FLAGS	= -O3 -fomit-frame-pointer -march=core2 -mpreferred-stack-boundary=4 -momit-leaf-frame-pointer -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -flto
	LD_FLAGS	= $(CC_FLAGS) -flto -O3 -lSDL 
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), opt64)
	CC_FLAGS	= -m64 -O3 -fomit-frame-pointer -march=core2 -momit-leaf-frame-pointer -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
	LD_FLAGS	= -O3 -lSDL
	EXTRA_OBJS	= main_pc.o
endif

LDFLAGS = $(LD_FLAGS) -Wall -Wextra
CCFLAGS = $(CC_FLAGS) -Wall -Wextra

OBJS	= $(EXTRA_OBJS) rt.o math64.o CPU.o MMU.o cp15.o mem.o RAM.o callout_RAM.o SoC.o pxa255_IC.o icache.o pxa255_UART.o

$(APP): $(OBJS)
	$(LD) -o $(APP) $(OBJS) $(LDFLAGS)
	$(EXTRA)

AVR:   $(APP)
	sudo avrdude -V -p ATmega328p -c avrisp2 -P usb -U flash:w:$(APP).hex:i

math64.o: math64.c math64.h types.h
	$(CC) $(CCFLAGS) -o math64.o -c math64.c

CPU.o: CPU.c CPU.h types.h math64.h icache.h
	$(CC) $(CCFLAGS) -o CPU.o -c CPU.c

icache.o: icache.c icache.h types.h CPU.h
	$(CC) $(CCFLAGS) -o icache.o -c icache.c

MMU.o: MMU.c MMU.h types.h
	$(CC) $(CCFLAGS) -o MMU.o -c MMU.c

cp15.o: cp15.c cp15.h CPU.h types.h
	$(CC) $(CCFLAGS) -o cp15.o -c cp15.c

mem.o: mem.c mem.h types.h
	$(CC) $(CCFLAGS) -o mem.o -c mem.c

avr_asm.o: avr_asm.S
	$(CC) $(CCFLAGS) -o avr_asm.o -c avr_asm.S

RAM.o: RAM.c RAM.h mem.h types.h
	$(CC) $(CCFLAGS) -o RAM.o -c RAM.c

callout_RAM.o: callout_RAM.c callout_RAM.h mem.h types.h
	$(CC) $(CCFLAGS) -o callout_RAM.o -c callout_RAM.c

SoC.o: SoC.c SoC.h RAM.h mem.h CPU.h MMU.h pxa255_IC.h math64.h icache.h
	$(CC) $(CCFLAGS) -o SoC.o -c SoC.c

pxa255_IC.o: pxa255_IC.c pxa255_IC.h mem.h CPU.h
	$(CC) $(CCFLAGS) -o pxa255_IC.o -c pxa255_IC.c

pxa255_UART.o: pxa255_UART.c pxa255_UART.h pxa255_IC.h mem.h CPU.h
	$(CC) $(CCFLAGS) -o pxa255_UART.o -c pxa255_UART.c

main_pc.o: SoC.h main_pc.c types.h
	$(CC) $(CCFLAGS) -o main_pc.o -c main_pc.c

main_avr.o: SoC.h main_avr.c types.h
	$(CC) $(CCFLAGS) -o main_avr.o -c main_avr.c

rt.o: rt.c types.h
	$(CC) $(CCFLAGS) -o rt.o -c rt.c

SD.o: SD.c SD.h types.h
	$(CC) $(CCFLAGS) -o SD.o -c SD.c

clean:
	rm -f $(APP) *.o


