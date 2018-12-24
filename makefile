CC=gcc
CFLAGS=-g -Wall -o
MESS=rm *.o lc3_test

lc3_test: lc3_test.c lc3.o lc3.h
	$(CC) lc3.o lc3_test.c $(CFLAGS) lc3_test

lc3.o: lc3.c lc3.h
	$(CC) -c lc3.c $(CFLAGS) lc3.o

clean:
	$(MESS)	
