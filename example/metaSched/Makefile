CC=gcc
PEDANTIC_PARANOID_FREAK =       -Wshadow -Wcast-align -Wnonnull\
                                -Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
                                -Wstrict-prototypes -Wredundant-decls -Wnested-externs \
                                -Wpointer-arith -Wwrite-strings -finline-functions -Wall
REASONABLY_CAREFUL_DUDE =       -Wall -Wstrict-prototypes -Wcast-align -Wmissing-prototypes\
				_Wnonnull -Wmissing-prototypes -Wmissing-declarations
WARNINGS=$(PEDANTIC_PARANOID_FREAK)
LDFLAGS=-L. -L/usr/local/lib/ -L../../lib/ -lsimgrid -lsimbatch
INCLUDES=-I ../../include
OPTS=-O0 -pipe
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
EXEC=metaSched

all: $(EXEC)

$(EXEC): $(OBJ)
	gcc -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)
	strip $(EXEC)

%.o: %.c
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)

.PHONY: clean mrproper

clean:	
	rm -f *.o *~ *.log *.out

mrproper: clean
	rm -f $(EXEC)
