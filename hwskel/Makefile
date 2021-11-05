CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11

all: homework

homework: homework.o
	$(CC) -o homework homework.o

homework.o: homework.c
	$(CC) $(CFLAGS) homework.c

test:
	python3 -m unittest -v test

clean:
	rm -f *.o homework
	rm -rf __pycache__
