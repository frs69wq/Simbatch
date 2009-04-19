CC=gcc
PEDANTIC_PARANOID_FREAK =       -Wshadow -Wcast-align -Wnonnull\
                                -Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
                                -Wstrict-prototypes -Wredundant-decls -Wnested-externs \
                                -Wpointer-arith -Wwrite-strings -finline-functions -Wall
REASONABLY_CAREFUL_DUDE =       -Wall -Wstrict-prototypes -Wcast-align -Wmissing-prototypes\
				_Wnonnull -Wmissing-prototypes -Wmissing-declarations
WARNINGS=$(PEDANTIC_PARANOID_FREAK)
SIMGRID_HOME=${HOME}/Library/Simgrid
SIMBATCH_HOME=${HOME}/Library/Simbatch
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
