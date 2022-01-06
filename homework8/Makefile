CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11 -O3

all: homework

homework: homework.o htable.o
	$(CC) -o homework homework.o htable.o

homework.o: homework.c
	$(CC) $(CFLAGS) homework.c

htable.o: htable.c htable.h
	$(CC) $(CFLAGS) htable.c

htable_test: htable.c htable.h htable_test.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 htable_test.c -o htable_test

test:
	python3 -m unittest -v test

clean:
	rm -f *.o homework htable_test
	rm -rf __pycache__
