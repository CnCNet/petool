BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CC         = i686-w64-mingw32-gcc
CFLAGS     = -m32 -std=c99 -pedantic -O2 -Wall -DREV=\"$(REV)\"
EXT       ?= .exe

TOOLS      = $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/petool$(EXT)

tools: $(EXES)

$(BUILD_DIR)/%$(EXT): src/%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(TOOLS)

.PHONY: tools clean
