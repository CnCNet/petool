TOOLS_DIR ?= .
BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CC        ?= gcc
CFLAGS     = -m32 -pedantic -O2 -Wall -DREV=\"$(REV)\"

tools: $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT)

$(BUILD_DIR)/linker$(EXT): $(TOOLS_DIR)/linker.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/extpe$(EXT): $(TOOLS_DIR)/extpe.c $(TOOLS_DIR)/pe.h
	$(CC) $(CFLAGS) -o $@ $<

clean_tools:
	rm -rf $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT)

.PHONY: tools clean_tools
