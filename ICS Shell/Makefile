CC := gcc

LIB := $(shell find lib -type f -name *.o)
SRC := $(shell find src -not -path '*/\.*' -type f -name *.c)
INC := -I include

DFLAGS := -g -DDEBUG -DCOLOR
CFLAGS := $(INC) $(DFLAGS) 


.PHONY: clean all setup

all: setup
	$(CC) $(CFLAGS) $(LIB) $(SRC) -o bin/53shell -lreadline

debug: setup
	$(CC) $(CFLAGS) $(DFLAGS) $(LIB) $(SRC) -o bin/53shell -lreadline

setup:
	mkdir -p bin

clean:
	$(RM) -r bin
