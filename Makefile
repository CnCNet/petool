TOOLS_DIR ?= .
BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CFLAGS     = -std=c99 -pedantic -O2 -Wall -Wextra -DREV=\"$(REV)\"

TOOLS      = $(foreach e,pe2obj patch,$(BUILD_DIR)/$(e)$(EXT))

tools: $(TOOLS)

$(BUILD_DIR)/%$(EXT): $(TOOLS_DIR)/src/%.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean: clean_tools
clean_tools:
	rm -rf $(TOOLS)

.PHONY: tools clean_tools clean
