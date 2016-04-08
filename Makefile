CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude
PROGRAMMER_TYPE = usbtiny

## The name of your project (without the .c)
# TARGET = blinkLED
## Or name it automatically after the enclosing directory
TARGET = $(lastword $(subst /, ,$(CURDIR)))

MCU   = atmega168a
DEVICE = __AVR_ATmega168A__
F_CPU = 8000000UL  
BAUD  = 9600UL

CPPFLAGS = -DCONF_WINC_USE_SPI -D$(DEVICE) -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I. -Iconfig -Iutils -Iutils/preprocessor -Iioport -Iclock -Idelay -Ispi -Ii2c -Iwinc
CFLAGS = -Os -g -std=gnu99 -Wall
## Use short (8-bit) data types 
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
## Splits up object files per function
CFLAGS += -ffunction-sections -fdata-sections 

LDFLAGS = -Wl,-Map,$(TARGET).map 
## Optional, but often ends up with smaller code
LDFLAGS += -Wl,--gc-sections 
## Relax shrinks code even more, but makes disassembly messy
## LDFLAGS += -Wl,--relax
## LDFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm  ## for floating-point printf
## LDFLAGS += -Wl,-u,vfprintf -lprintf_min      ## for smaller printf

TARGET_ARCH = -mmcu=$(MCU)

OBJECTS = main.o sysclk.o nmspi.o spi.o \
          i2c.o eeprom.o nm_bsp_mega.o \
          nm_common.o nm_bus_wrapper_mega.o \
          m2m_hif.o m2m_periph.o m2m_wifi.o \
          nmasic.o nmbus.o nmdrv.o nmi2c.o \
          nmuart.o spi_flash.o socket.o

winc_drvr : $(OBJECTS)

main.o : WINC_mega.c config/main.h \
         utils/interrupt.h \
         utils/interrupt/interrupt_avr8.h \
         utils/preprocessor/preprocessor.h \
         winc/common/include/nm_common.h \
         ioport/ioport.h \
         clock/sysclk.h \
         delay/delay.h \
         spi/spi.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

sysclk.o : clock/mega/sysclk.c clock/mega/sysclk.h \
           config/board.h \
           compiler.h \
           utils/parts.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

spi.o : spi/spi.c spi/spi.h \
        config/main.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

i2c.o : i2c/i2c.c i2c/i2c.h \
        config/main.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

eeprom.o : eeprom/eeprom.c eeprom/eeprom.h \
           utils/interrupt.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nm_bsp_mega.o : winc/bsp/source/nm_bsp_mega.c \
                winc/bsp/include/nm_bsp.h \
                winc/common/include/nm_common.h \
                config/main.h \
                config/conf_winc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nm_common.o : winc/common/source/nm_common.c \
              winc/common/include/nm_common.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nm_bus_wrapper_mega.o : winc/bus_wrapper/source/nm_bus_wrapper_mega.c \
                        winc/bsp/include/nm_bsp.h \
                        winc/common/include/nm_common.h \
                        winc/bus_wrapper/include/nm_bus_wrapper.h \
                        config/main.h \
                        config/conf_winc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

m2m_hif.o : winc/driver/source/m2m_hif.c \
            winc/common/include/nm_common.h \
            winc/common/include/nm_debug.h \
            winc/driver/source/nmbus.h \
            winc/bsp/include/nm_bsp.h \
            winc/bsp/include/nm_bsp_mega.h \
            winc/driver/source/m2m_hif.h \
            winc/driver/include/m2m_types.h \
            winc/driver/source/nmasic.h \
            winc/driver/include/m2m_periph.h \
            winc/bus_wrapper/include/nm_bus_wrapper.h \
            config/conf_winc.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

m2m_periph.o : winc/driver/source/m2m_periph.c \
               winc/driver/include/m2m_periph.h \
               winc/common/include/nm_common.h \
               winc/driver/source/nmasic.h \
               winc/driver/source/m2m_hif.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

m2m_wifi.o : winc/driver/source/m2m_wifi.c \
             winc/driver/include/m2m_wifi.h \
             winc/driver/source/m2m_hif.h \
             winc/driver/source/nmasic.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nmasic.o : winc/driver/source/nmasic.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;


nmbus.o : winc/driver/source/nmbus.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nmdrv.o : winc/driver/source/nmdrv.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nmi2c.o : winc/driver/source/nmi2c.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nmspi.o : winc/driver/source/nmspi.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

nmuart.o : winc/driver/source/nmuart.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

spi_flash.o : winc/spi_flash/spi_flash.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

socket.o : winc/socket/source/socket.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

%.hex: %.elf
	 $(OBJCOPY) -j .text -j .data -O ihex $< $@

%.eeprom: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ 

%.lst: %.elf
	$(OBJDUMP) -S $< > $@

## These targets don't have files named after them
.PHONY: all size clean

all: $(TARGET).hex 

# Optionally show how big the resulting program is 
size:  $(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(TARGET).elf

clean:
	rm -f winc_drvr $(OBJECTS)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM WINC_mega.c

flash: $(TARGET).hex 
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p m168 $(PROGRAMMER_ARGS) -U flash:w:$<
