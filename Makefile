# Ajastin Makefile
# Public domain, use as you wish, nothing important here
#
# I used this as a guidance:
# http://hackaday.com/2016/03/15/
# embed-with-elliot-microcontroller-makefiles/
# and this:
# https://github.com/hexagon5un/AVR-Programming/blob/master/
# Chapter02_Programming-AVRs/blinkLED/Makefile
# These links are provided just for your convinience, if you want to
# create your own makefiles.

# Ajastin
TARGET = ajastin
OBJECTS = ajastin.o
HEADERS =

# AtTiny
MCU = attiny85
F_CPU = 1000000UL
LFUSE = 0x62
HFUSE = 0xDF
EFUSE = 0xFF
PROGRAMMER = usbtiny

# Programs
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Flags
AVRDUDEFLAGS = -c $(PROGRAMMER) -p $(MCU)
CPPFLAGS = -DF_CPU=$(F_CPU) -I.
CFLAGS = -Os -g -std=gnu99 -Wall
LDFLAGS = -Wl,-Map,$(TARGET).map,--gc-sections -mmcu=$(MCU)

# Rules
all: flash

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDEFLAGS) -U flash:w:$(TARGET).hex

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c $(HEADERS) Makefile
	 $(CC) $(CFLAGS) $(CPPFLAGS) -mmcu=$(MCU) -c -o $@ $<;

clean:
	rm -f $(TARGET).{elf,hex,map}

program: flash

build: $(TARGET).hex

fuse:
	$(AVRDUDE) $(AVRDUDEFLAGS) -U lfuse:w:$(LFUSE):m \
		-U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m
