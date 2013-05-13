BUILD_DIR ?= .

REV        = $(shell sh -c 'git rev-parse --short @{0}')
CC        ?= gcc
CFLAGS     = -std=gnu99 -m32 -pedantic -O2 -Wall -DREV=\"$(REV)\"

tools: $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT) $(BUILD_DIR)/modpe$(EXT)

$(BUILD_DIR)/linker$(EXT): linker.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/extpe$(EXT): extpe.c pe.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/modpe$(EXT): modpe.c pe.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/linker$(EXT) $(BUILD_DIR)/extpe$(EXT) $(BUILD_DIR)/modpe$(EXT)

.PHONY: tools clean
