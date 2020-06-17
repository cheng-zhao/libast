CC = gcc
LIBS = -lm
CFLAGS = -std=c99 -O3 -Wall
EXEC = libast_example

all:
	$(CC) $(CFLAGS) -o $(EXEC) ../libast.c example.c -I.. $(LIBS)

clean:
	rm $(EXEC)
