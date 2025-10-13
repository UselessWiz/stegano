CC = gcc
CFLAGS = -ansi -Wall -Werror
OUTDIR = bin

stegano.out: main.o stegano.o
	$(CC) $(OUTDIR)/main.o $(OUTDIR)/stegano.o -o $(OUTDIR)/stegano.out -lm

main.o: main.c stegano.h
	$(CC) $(CFLAGS) -c main.c -o $(OUTDIR)/main.o

stegano.o: stegano.c
	$(CC) $(CFLAGS) -c stegano.c -o $(OUTDIR)/stegano.o

clean: 
	rm -f bin
	