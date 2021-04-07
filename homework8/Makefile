CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11

all: homework

homework: homework.o
	$(CC) -shared -o libotushomework8.so homework.o

homework.o: homework.c
	$(CC) $(CFLAGS) -fPIC homework.c

test:
	echo "Homework8 hasn't got tests"

clean:
	rm -f *.o *.so homework
	rm -rf __pycache__
