# Sample makefile

CC = gcc
CFLAGS = -Wall -Werror -ansi -lm
BMP = new.bmp

program: stegano.o main.o
	$(CC) $(CFLAGS) -o program stegano.o main.o

stegano.o: stegano.h stegano.c
	$(CC) $(CFLAGS) -c -o stegano.o stegano.c

main.o:
	$(CC) $(CFLAGS) -c -o main.o main.c

clean:
	rm $(BMP) program stegano.o main.o
