# uARM

<h3>This is broken version. New version will come out later.<h3>

Run Linux (Ubuntu) on your AVR with the help of emulator<br>
A fork of Dmitry Grinberg's project at http://dmitry.gr/?r=05.Projects&proj=07.%20Linux%20on%208bit

(This readme will be fixed later)

<b>This project is in testing progress, so it might not stable now!</b>

## Intro
I was dream of running Linux on my little atmega328p (which has 32KB of flash and 2KB of RAM) since Dmitry Grinberg ran Linux on his atmega1284 (which has 128KB of flash and 16KB of ram!). I first read his article about 2 or 3 years ago and until now, I have successfully reduced the emulator's size to fit an atmega328p.

## Goals
This project's goals is to run Linux on atmega328p (or Arduino UNO) based on Dmitry Grinberg's uARM emulator running on atmega1284p.
As you can see below, the hex size is actually fit the atmega328p flash and ram size!

To build, run ```make BUILD=avr```

To check the size of the hex, use ```avr-size -C --mcu=atmega328p uARM```

Build output:

```
AVR Memory Usage
----------------
Device: atmega328p

Program:   31416 bytes (95.9% Full)
(.text + .data + .bootloader)

Data:       1737 bytes (84.8% Full)
(.data + .bss + .noinit)
```

You can get a working Linux image [here](https://mega.nz/#!0wQSSSbT!VHOZ3XhWDeyxMb8yvuCBw-GfJkpYNnnjIgl9SKWLi6Q). Copy the image to SD card using ```dd```, and enjoy!
