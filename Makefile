REV     ?= $(shell git rev-parse --short @{0})
CC      ?= gcc
RM      ?= rm -f
CFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -DREV=\"$(REV)\"

ifdef DEBUG
CFLAGS  += -ggdb
else
CFLAGS  += -O2
endif

all: petool

petool: $(wildcard src/*.c)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	$(RM) petool
