#---------------------------------------------------------------
#             CONFIGURE THESE VARIABLES IF NEEDED
#---------------------------------------------------------------

ROOT = ${HOME}/compiladores/root
RTS_LIB_DIR = $(ROOT)/usr/lib

COMPILER=../l22
ASM=yasm

#---------------------------------------------------------------
#    PROBABLY, NO NEED TO CHANGE ANYTHING BEYOND THIS POINT
#---------------------------------------------------------------

COMPILER_FLAGS = 
LDFLAGS  = -m elf_i386 -L$(RTS_LIB_DIR) -lrts

SOURCES     = $(wildcard *tests/.l22)
OBJECTS     = $(SOURCES:%.l22=%.o)
EXECUTABLES = $(SOURCES:%.l22=%)

#---------------------------------------------------------------
#                DO NOT CHANGE AFTER THIS LINE
#---------------------------------------------------------------

all: $(EXECUTABLES)

%.c: %.l22
	$(COMPILER) $< -o $@

%.asm: %.l22
	$(COMPILER) --target asm $<

%.o: %.asm
	$(ASM) -felf32 $< -o $@

%:%.o
	$(LD) $< -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJECTS) $(EXECUTABLES) *.c *.asm

#---------------------------------------------------------------
#                           THE END
#---------------------------------------------------------------
