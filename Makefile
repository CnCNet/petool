TOOLS_DIR ?= .
BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CFLAGS     = -m32 -std=c99 -pedantic -O2 -Wall -Wextra -DREV=\"$(REV)\"

# for windows compile
CC         = i686-w64-mingw32-gcc
EXT       ?= .exe

TOOLS      = $(foreach e,linker petool pe2obj patch,$(BUILD_DIR)/$(e)$(EXT))

tools: $(TOOLS)

$(BUILD_DIR)/%$(EXT): $(TOOLS_DIR)/src/%.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(TOOLS)

.PHONY: tools clean
