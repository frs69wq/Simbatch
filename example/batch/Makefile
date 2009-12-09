CC=gcc
WARNINGS=-Wall -Wextra
SIMGRID_HOME=${HOME}/myroot
SIMBATCH_HOME=../..
INCLUDES=-I$(SIMGRID_HOME)/include -I${SIMBATCH_HOME}/include 
LD_PATH=-L. -L$(SIMGRID_HOME)/lib/ -L$(SIMBATCH_HOME)/lib
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
