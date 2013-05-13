BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CC         = i686-w64-mingw32-gcc
CFLAGS     = -m32 -std=c99 -pedantic -O2 -Wall -DREV=\"$(REV)\"
EXT       ?= .exe

tools: $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT) $(BUILD_DIR)/modpe$(EXT)

$(BUILD_DIR)/%$(EXT): %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT) $(BUILD_DIR)/modpe$(EXT)

.PHONY: tools clean
