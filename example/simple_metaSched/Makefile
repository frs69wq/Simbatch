CC=gcc
LDFLAGS=-L. -L/usr/local/lib/ -L../../lib/ -L/home/gcharrie/myroot/lib/ -lsimgrid -lsimbatch -lwld
INCLUDES=-I ../../include -I /home/gcharrie/myroot/include
OPTS=-O0 -pipe
DEBUG= # -pg -DDEBUG
CFLAGS=$(WARNINGS) $(OPTS) $(DEBUG)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
EXEC=emample

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
