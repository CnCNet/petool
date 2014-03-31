CC	?= gcc
RM	?= rm -f
CFLAGS	 = -O2 -std=c99 -pedantic -Wall -Wextra -DREV=\"$(shell git rev-parse --short @{0})\"

ifdef DEBUG
CFLAGS	+= -ggdb
endif

all: petool$(EXT)

petool$(EXT): src/*.c
	$(CC) $(CFLAGS) -o $@ $?

clean:
	$(RM) petool$(EXT)

.PHONY: clean
