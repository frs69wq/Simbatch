CC=gcc
LDFLAGS=-L. -L/usr/local/lib/ -L../../lib/ -L/home/gcharrie/myroot/lib/ -lsimgrid -lsimbatch
INCLUDES=-I ../../include -I /home/gcharrie/myroot/include
OPTS=-O2 -pipe -g -W -Wall
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG)
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
	rm -f *.o *~ *.log *.out

mrproper: clean
	rm -f $(EXEC)
