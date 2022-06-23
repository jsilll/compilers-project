ROOT = ${HOME}/compiladores/root
RTS_LIB_DIR = $(ROOT)/usr/lib
COMPILER= ../l22
ASM= yasm
LD= ld

LDFLAGS = -melf_i386 -L$(RTS_LIB_DIR) -lrts
SOURCES = $(wildcard ./src/*.l22)
OBJECTS = $(SOURCES:%.l22=%.o)
EXECUTABLES = $(SOURCES:%.l22=%.exe)
ASMS = $(SOURCES:%.l22=%.asm)
XMLS = $(SOURCES:%.l22=%.xml)

all: $(EXECUTABLES)

xml: $(XMLS)

asm: $(ASMS)

%.o: %.asm
	$(ASM) -felf32 $< -o $@

%.asm: %.l22
	$(COMPILER) --target asm $< -o $@

%.xml: %.l22
	$(COMPILER) --target xml $< -o $@

%.exe:%.o
	$(LD) $< -o $@ $(LDFLAGS)

clean:
	$(info Cleaning ...)
	@$(RM) $(OBJECTS) $(EXECUTABLES) $(ASMS) $(XMLS) src/*.out