MCU=atmega32
CFLAGS=-g -Wall -mcall-prologues -mmcu=$(MCU) -Os -DF_CPU=8000000UL
LDFLAGS=-Wl,-gc-sections -Wl,-relax
CC=avr-gcc
TARGET_1=aero
TARGET_2=manette
PROGRAMMER=stk500

all: $(TARGET_1).hex $(TARGET_2).hex

clean:
	rm -f *.o *.elf *.hex *.h.gch

%.hex: %.elf
	avr-objcopy -R .eeprom -O ihex $< $@

$(TARGET_1).elf: $(TARGET_1).o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ lcd.c utils.c fifo.c uart.c driver.c util_29.c -o $@

$(TARGET_2).elf: $(TARGET_2).o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ lcd.c utils.c fifo.c uart.c driver.c util_29.c -o $@

a: $(TARGET_1).hex
	avrdude -c $(PROGRAMMER) -P /dev/ttyACM0 -p $(MCU) -b 19200 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m -U flash:w:$<:i

m: $(TARGET_2).hex
	avrdude -c $(PROGRAMMER) -P /dev/ttyACM0 -p $(MCU) -b 19200 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m -U flash:w:$<:i
