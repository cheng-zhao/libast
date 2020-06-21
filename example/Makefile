CC = gcc
LIBS = -lm
CFLAGS = -std=c99 -O3 -Wall
SRC1 = parse_file.c
SRC2 = draw_tree.c
EXEC1 = libast_parse
EXEC2 = libast_draw

all: $(EXEC1) $(EXEC2)

$(EXEC1):
	$(CC) $(CFLAGS) -o $(EXEC1) ../libast.c $(SRC1) -I.. $(LIBS)

$(EXEC2):
	$(CC) $(CFLAGS) -o $(EXEC2) ../libast.c $(SRC2) -I.. $(LIBS)

clean:
	rm $(EXEC1) $(EXEC2)
