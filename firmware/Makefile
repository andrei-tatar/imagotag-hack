FLASH_SIZE = 0x7FFF # 32KB flash

XRAM_START = 0xF000
XRAM_SIZE = 0xF00 # slow RAM

IRAM_SIZE = 0x100 # fast RAM

SHELL = sh -xv
SRC = $(shell find . -type f -name "*.c")
REL = $(patsubst %.c, %.rel, $(SRC)) 
ASM = $(patsubst %.c, %.asm, $(SRC)) 
LST = $(patsubst %.c, %.lst, $(SRC)) 
SYM = $(patsubst %.c, %.sym, $(SRC)) 
RST = $(patsubst %.c, %.rst, $(SRC)) 

CC = sdcc
SDCC_FLAGS = --model-small --opt-code-speed
LDFLAGS_FLASH = \
--out-fmt-ihx \
--code-loc 0x000 --code-size $(FLASH_SIZE) \
--xram-loc $(XRAM_START) --xram-size $(XRAM_SIZE) \
--iram-size $(IRAM_SIZE)

TARGET = firmware

all: $(TARGET).hex

%.rel : %.c
	$(CC) -c $(SDCC_FLAGS) -DBUILD -c -o $*.rel $<

$(TARGET).hex: $(REL) Makefile
	$(CC) $(LDFLAGS_FLASH) $(SDCC_FLAGS) -o $(TARGET).hex $(REL) 

clean:
	rm -f $(REL) $(ASM) $(LST) $(SYM) $(RST) $(TARGET).lk $(TARGET).map $(TARGET).mem

.PHONY: clean 
