CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11

all: homework

homework: homework.o encode.o
	$(CC) -o homework encode.o homework.o

homework.o: homework.c
	$(CC) $(CFLAGS) homework.c

encode.o: encode.c encode.h
	$(CC) $(CFLAGS) encode.c

test:
	python3 -m unittest -v test

clean:
	rm -f *.o homework
	rm -rf __pycache__
