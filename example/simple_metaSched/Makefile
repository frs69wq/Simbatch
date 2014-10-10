CC=gcc
WARNINGS=-Wall -Wextra
SIMGRID_HOME=/usr/local
SIMBATCH_HOME=../../
INCLUDES=-I$(SIMGRID_HOME)/include -I${SIMBATCH_HOME}/include 
LD_PATH= -L$(SIMGRID_HOME)/lib/ 
LD_FLAGS=-lsimgrid -lsimbatch -lwld
OPTS=-O2 -pipe
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
EXEC=simple_metasched

all: $(EXEC)

$(EXEC): $(OBJ)
	gcc -o $@ $(OBJ) $(LD_PATH) $(LD_FLAGS) $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(INCLUDES) $(CFLAGS)

.PHONY: clean mrproper

clean:	
	rm -f *.o *~ *.log *.out

mrproper: clean
	rm -f $(EXEC)
