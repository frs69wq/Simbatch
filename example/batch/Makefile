CC=gcc
WARNINGS=-Wall -Wextra
SIMGRID_HOME=${HOME}/Library/MyLibs/simgrid
SIMBATCH_HOME=../../lib
INCLUDES=-I$(SIMGRID_HOME)/include -I../../include 
LD_PATH=-L. -L$(SIMGRID_HOME)/lib/ -L$(SIMBATCH_HOME)
LD_FLAGS=-lsimgrid -lsimbatch
OPTS=-O0 -pipe
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
EXEC=batch

all: $(EXEC)

$(EXEC): $(OBJ)
	gcc -o $@ $(OBJ) $(LD_PATH) $(LD_FLAGS) $(CFLAGS)
	strip $(EXEC)

%.o: %.c
	$(CC) -c $< $(INCLUDES) $(CFLAGS)

.PHONY: clean mrproper

clean:	
	rm -f *.o *~ *log *out

mrproper: clean
	rm -f $(EXEC)
