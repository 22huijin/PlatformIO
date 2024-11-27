## Project Name
PROJECT = jkit-128-template
TARGET = $(PROJECT).elf

## General Flags
MCU = atmega128
CC = avr-gcc
CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
## -Wall: Enable all warnings
## -std=gnu99: Use the GNU C99 extensions
## -Os: Optimize for size
## -fpack-struct: Pack structure members together without holes(data structure alignment)
CFLAGS = $(COMMON)
CFLAGS += -Wall -std=gnu99 -Os -fpack-struct
## For print float
CFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm

## Linker flags
## -Wl, -Map: Create a link map
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-u,vfprintf,-Map=$(PROJECT).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Libraries
LIBS = -lm -lprintf_flt
LIBDIRS = 
INCLUDES = 

## Objects
OBJECTS = main.o
LINKONLYOBJECTS =

## Build
all: $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss

## Compile
%.o: %.c
    $(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

## Link
$(TARGET): $(OBJECTS)
    $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $@

## Create flash image
%.hex: $(TARGET)
    avr-objcopy -O ihex $(HEX_FLASH_FLAGS) $< $@

## Create eeprom image
%.eep: $(TARGET)
    -avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

## Create extended listing file from ELF output file
%.lss: $(TARGET)
    avr-objdump -h -S $< > $@

## Clean target
.PHONY: clean
clean:
    -rm -rf $(OBJECTS) $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).map

## Comment
# 새로운 Object 파일이 필요한 경우(.c 파일이 추가된 경우) ㅒOBJECTS에 다음과 같이 추가한다.
# OBJECTS = main.o new.o
#
# 바이러니 파일 이름은 PROJECT에 지정한다.