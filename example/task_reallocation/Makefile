CC=gcc
LDFLAGS=-L. -L/usr/local/lib/ -L/home/gcharrie/sources/Simbatch/lib/ -L/home/gcharrie/myroot/lib/ -lsimgrid -lsimbatch
INCLUDES=-I /home/gcharrie/sources/Simbatch/include -I /home/gcharrie/myroot/include
OPTS=-O2 -pipe -g -W -Wall
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG) -DMCT #-DRESCHED -DVERBOSE -DTRANSFERT_TIME
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
EXEC=example

all: $(EXEC)

$(EXEC): $(OBJ) 
	gcc -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)

.PHONY: clean mrproper

clean:	
	rm -f *.o *~ *.log *.out realloc.txt temps.txt

mrproper: clean
	rm -f $(EXEC)
