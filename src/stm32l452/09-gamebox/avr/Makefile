# Hey Emacs, this is a -*- makefile -*-

# AVR-GCC Makefile template, derived from the WinAVR template (which
# is public domain), believed to be neutral to any flavor of "make"
# (GNU make, BSD make, SysV make)

# Modyfied by Malte Marwedel, so that it works with the sp12 programmer

MCU = atmega32
FORMAT = ihex
TARGET = main
SRC = $(TARGET).c ../gameoflife.c ../graphicfunctions.c graphicint.c ../text.c ../demo.c ../menu.c userinput.c ../prace.c ../ppong.c ../psnake.c ../ptetris.c ../pxxo.c ../sram.c ../highscore.c ../other.c timings.c ../prev.c ../xmas.c
ASRC =
OPT = -Os -ffast-math -fweb -Winline
# you may use the -frename-registers parameter for code optimization as well
# but it does not seem to always reduce the code size

# Name of this Makefile (used for "make depend").
MAKEFILE = Makefile

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Place -D or -U options here
CDEFS =

# Place -I options here
CINCS =


CDEBUG = -g$(DEBUG)
CWARN = -Wall -Wstrict-prototypes
CTUNING = -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
#CEXTRA = -Wa,-adhlns=$(<:.c=.lst)
CFLAGS = $(CDEBUG) $(CDEFS) $(CINCS) $(OPT) $(CWARN) $(CSTANDARD) $(CEXTRA)


#ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs


#Additional libraries.

# Minimalistic printf version
PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min

# Floating point printf version (requires MATH_LIB = -lm below)
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt

PRINTF_LIB =

# Minimalistic scanf version
SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min

# Floating point + %[ scanf version (requires MATH_LIB = -lm below)
SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt

SCANF_LIB =

MATH_LIB = -lm

# External memory options

# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# used for variables (.data/.bss) and heap (malloc()).
#EXTMEMOPTS = -Wl,-Tdata=0x801100,--defsym=__heap_end=0x80ffff

# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# only used for heap (malloc()).
#EXTMEMOPTS = -Wl,--defsym=__heap_start=0x801100,--defsym=__heap_end=0x80ffff

EXTMEMOPTS =

#LDMAP = $(LDFLAGS) -Wl,-Map=$(TARGET).map,--cref
LDFLAGS = $(EXTMEMOPTS) $(LDMAP) $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)

CC = avr-gcc
#CC = gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
MV = mv -f
UPLOAD  = avrbootloader/SP12v2_1.1/sp12

# Define all object files.
OBJ = $(SRC:.c=.o) $(ASRC:.S=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(SRC:.c=.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)


# Default target.
all: build

build: clean elf hex eep
	avr-size $(TARGET).elf

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym

upload:
	 $(UPLOAD) -M4 -wpfC $(TARGET).hex -T1

uploadr:
	$(UPLOAD) -M4 -wpfC $(TARGET).hex

uploadc:
	$(UPLOAD) -M4 -wpf $(TARGET).hex -T1

removebackup:
	rm *.h~
	rm *.c~

# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
--change-section-address .data-0x800000 \
--change-section-address .bss-0x800000 \
--change-section-address .noinit-0x800000 \
--change-section-address .eeprom-0x810000


coff: $(TARGET).elf
	$(COFFCONVERT) -O coff-avr $(TARGET).elf $(TARGET).cof


extcoff: $(TARGET).elf
	$(COFFCONVERT) -O coff-ext-avr $(TARGET).elf $(TARGET).cof


.SUFFIXES: .elf .hex .eep .lss .sym

.elf.hex:
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.eep:
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
	$(NM) -n $< > $@



# Link: create ELF output file from object files.
$(TARGET).elf: $(OBJ)
	$(CC) $(ALL_CFLAGS) $(OBJ) --output $@ $(LDFLAGS)


# Compile: create object files from C source files.
.c.o:
	$(CC) -c $(ALL_CFLAGS) $< -o $@


# Compile: create assembler files from C source files.
.c.s:
	$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
.S.o:
	$(CC) -c $(ALL_ASFLAGS) $< -o $@



# Target: clean project.
clean:
	$(REMOVE) $(TARGET).hex $(TARGET).eep $(TARGET).cof $(TARGET).elf \
	$(TARGET).map $(TARGET).sym $(TARGET).lss \
	$(OBJ) $(LST) $(SRC:.c=.s) $(SRC:.c=.d)

depend:
	if grep '^# DO NOT DELETE' $(MAKEFILE) >/dev/null; \
	then \
		sed -e '/^# DO NOT DELETE/,$$d' $(MAKEFILE) > \
			$(MAKEFILE).$$$$ && \
		$(MV) $(MAKEFILE).$$$$ $(MAKEFILE); \
	fi
	echo '# DO NOT DELETE THIS LINE -- make depend depends on it.' \
		>> $(MAKEFILE); \
	$(CC) -M -mmcu=$(MCU) $(CDEFS) $(CINCS) $(SRC) $(ASRC) >> $(MAKEFILE)

.PHONY:	all build elf hex eep lss sym program coff extcoff clean depend
