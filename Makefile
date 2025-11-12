CC = gcc
CFLAGS = -Wall -Wextra -I. -g3

INCLUDE = secded.h
SOURCE = secded.c
TEST = test_secded.c

.PHONY: clean check

check: test_secded
	./test_secded

test_secded: secded.o test_secded.o
	$(CC) $(CFLAGS) $^ -o $@

secded.o: secded.c secded.h
	$(CC) $(CFLAGS) $< -c -o $@

test_secded.o: test_secded.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o test_secded
