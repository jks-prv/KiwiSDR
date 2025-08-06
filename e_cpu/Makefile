#
# assembler for the embedded processor
#
# Copyright (c) 2013-2025 John Seamons, ZL4VO/KF6VO


include ../Makefile.comp.inc

BUILD_DIR = ../../build
TOOLS_DIR = $(BUILD_DIR)/tools
GEN_DIR = $(BUILD_DIR)/gen

CFLAGS = -I. -I./asm -I.. -D_GNU_SOURCE -g 
ifeq ($(ASAN),1)
    CFLAGS += -fsanitize=address
endif

ASM = $(TOOLS_DIR)/asm

all: $(ASM) no_gen

gen_includes debug2: $(ASM)
	-$(ASM) -o $(GEN_DIR) $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

info: $(ASM)
	-$(ASM) -i -o $(GEN_DIR) $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

gen_other: $(ASM)
	-$(ASM) -o $(GEN_DIR) -g $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

$(TOOLS_DIR):
	@mkdir -p $(TOOLS_DIR)

$(GEN_DIR):
	@mkdir -p $(GEN_DIR)

ASM_INCLUDES = $(wildcard asm/*.h)
ASM_SOURCE = $(wildcard asm/*.cpp)

$(ASM): $(TOOLS_DIR) $(GEN_DIR) $(ASM_INCLUDES) $(ASM_SOURCE) Makefile
	$(CPP) $(CFLAGS) $(ASM_SOURCE) -o $@

# don't regenerate the .h/.vh files
no_gen: $(ASM)
	-$(ASM) -n -o $(GEN_DIR) $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

debug: $(ASM)
	-$(ASM) -n -d -o $(GEN_DIR) $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

binary: $(ASM)
	-$(ASM) -n -b -o $(GEN_DIR) $(OTHER_DIR)
	sum $(GEN_DIR)/kiwi.aout

stat stats: $(ASM)
	-$(ASM) -n -s -o $(GEN_DIR) $(OTHER_DIR)

gdb: $(ASM)
	@echo "gdb: r -o ../../build/gen"
	gdb $(ASM)

clean:
	-rm -rf $(TOOLS_DIR)/asm* $(GEN_DIR)
