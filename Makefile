TOOLS_DIR ?= .
BUILD_DIR ?= .

REV_TOOLS  = $(shell sh -c 'cd $(TOOLS_DIR); git rev-parse --short @{0}')
CFLAGS     = -std=c99 -pedantic -Wall -Wextra -DREV=\"$(REV_TOOLS)\"
TOOLS	   = $(BUILD_DIR)/petool$(EXT)

ifdef DEBUG
CFLAGS    += -ggdb
else
CFLAGS    += -O2
endif

tools: $(TOOLS)

$(BUILD_DIR)/petool$(EXT): $(TOOLS_DIR)/src/*.c
	$(CC) $(CFLAGS) -o $@ $?

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean_tools:
	rm -rf $(TOOLS)

.PHONY: tools clean_tools
