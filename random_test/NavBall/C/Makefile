CC=gcc
CFLAGS=-Wall -Wextra -pedantic -g -std=gnu99 -O3
SANITIZERS=-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=leak
LIBS= -lm -pthread -lSDL2 

DEPMAIN=src/main.o src/navball.o src/textureMap.o

all: clean main

main: $(DEPMAIN)
	$(CC) -o $@ $^ $(CFLAGS) $(SANITIZERS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) $(SANITIZERS) $(LIBS) -c $^ -o $@

SUBDIR_ROOTS := src
DIRS := . $(shell find $(SUBDIR_ROOTS) -type d)
GARBAGE_PATTERNS := *.o
GARBAGE := $(foreach DIR,$(DIRS),$(addprefix $(DIR)/,$(GARBAGE_PATTERNS)))

clean:
	rm -rf $(GARBAGE)
	rm -f main
