CC = gcc
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> encode-decode
CFLAGS = -ansi -Wall -Werror
OUTDIR = bin

stegano.out: $(OUTDIR)/main.o $(OUTDIR)/stegano.o
	$(CC) $(OUTDIR)/main.o $(OUTDIR)/stegano.o -o $(OUTDIR)/stegano.out -lm

$(OUTDIR)/main.o: $(OUTDIR) main.c stegano.h
	$(CC) $(CFLAGS) -c main.c -o $(OUTDIR)/main.o

$(OUTDIR)/stegano.o: $(OUTDIR) stegano.c
	$(CC) $(CFLAGS) -c stegano.c -o $(OUTDIR)/stegano.o

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean: 
	rm -rf bin
<<<<<<< HEAD
	
=======
CFLAGS = -Wall -Werror -ansi

main.exe: main.o stegano.o
	$(CC) $(CFLAGS) -o main.exe main.o stegano.o -lm
>>>>>>> compression-decompression
=======
	
>>>>>>> encode-decode
