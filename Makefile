TOOLS_DIR ?= .
BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CFLAGS     = -g -std=c99 -pedantic -O2 -Wall -Wextra -DREV=\"$(REV)\"
TOOLS	   = $(BUILD_DIR)/petool$(EXT)

tools: $(TOOLS)

$(BUILD_DIR)/petool$(EXT): $(TOOLS_DIR)/src/*.c
	$(CC) $(CFLAGS) -o $@ $?

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean: clean_tools
clean_tools:
	rm -rf $(TOOLS)

.PHONY: tools clean_tools clean
