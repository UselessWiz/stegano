CC = gcc
CFLAGS = -Wall -Werror -ansi

main.exe: main.o stegano.o
	$(CC) $(CFLAGS) -o main.exe main.o stegano.o -lm