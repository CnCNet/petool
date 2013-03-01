TOOLS_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CC        ?= gcc
CFLAGS     = -m32 -pedantic -O2 -Wall -DREV=\"$(REV)\"

tools: $(TOOLS_DIR)/linker$(EXT) $(TOOLS_DIR)/extpe$(EXT)

$(TOOLS_DIR)/linker$(EXT): $(TOOLS_DIR)/linker.c
	$(CC) $(CFLAGS) -o $(TOOLS_DIR)/linker$(EXT) $(TOOLS_DIR)/linker.c

$(TOOLS_DIR)/extpe$(EXT): $(TOOLS_DIR)/extpe.c $(TOOLS_DIR)/pe.h
	$(CC) $(CFLAGS) -o $(TOOLS_DIR)/extpe$(EXT)  $(TOOLS_DIR)/extpe.c

clean_tools:
	rm -rf $(TOOLS_DIR)/linker$(EXT) $(TOOLS_DIR)/extpe$(EXT)

.PHONY: tools clean_tools
