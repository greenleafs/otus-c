CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11 -g

all: homework

homework: homework.o htable.o
	$(CC) -o homework homework.o htable.o

homework.o: homework.c
	$(CC) $(CFLAGS) homework.c

htable.o: htable.c htable.h
	$(CC) $(CFLAGS) htable.c

test:
	python3 -m unittest -v test

clean:
	rm -f *.o homework
	rm -rf __pycache__
