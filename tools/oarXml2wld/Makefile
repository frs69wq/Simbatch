CC=gcc
PEDANTIC_PARANOID_FREAK =       -Wshadow -Wcast-align -Wnonnull\
                                -Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
                                -Wstrict-prototypes -Wredundant-decls -Wnested-externs \
                                -Wpointer-arith -Wwrite-strings -finline-functions -Wall
WARNINGS=$(PEDANTIC_PARANOID_FREAK)
OPTS=-O0 -pipe
DEBUG=-DDEBUG
CFLAGS= -g $(WARNINGS) $(OPTS) $(DEBUG)
LDFLAGS=-L/usr/lib -lxml2
INCLUDES=-I /usr/include/libxml2
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
BIN=oarxml2wld

all: $(BIN)

$(BIN): $(OBJ)
	gcc -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)
	strip $(BIN)

%.o: %.c
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)

.PHONY: clean mrproper

clean:	
	rm -f *.o *~ *log *out oarxml2wld

mrproper: clean
	rm -f $(BIN)
