CC = gcc
CFLAGS = -Wall -Wextra -I. -O0 -fno-strict-aliasing

INCLUDE = secded.h
SOURCE = secded.c
TEST = test_secded.c

.PHONY: clean check

check: test_secded
	./test_secded

speed_test: speed_test_with_restrict speed_test_without_restrict
	time ./speed_test_with_restrict
	time ./speed_test_without_restrict

test_secded: secded.o test_secded.o
	$(CC) $(CFLAGS) $^ -o $@

secded.o: secded.c secded.h
	$(CC) $(CFLAGS) $< -c -o $@

test_secded.o: test_secded.c
	$(CC) $(CFLAGS) $< -c -o $@

speed_test_with_restrict : speed_test_with_restrict.o secded.o
	$(CC) $(CFLAGS) $^ -o $@

speed_test_without_restrict : speed_test_without_restrict.o secded.o
	$(CC) $(CFLAGS) $^ -o $@

speed_test_with_restrict.o: speed_test_with_restrict.c
	$(CC) $(CFLAGS) $< -c -o $@

speed_test_without_restrict.o : speed_test_without_restrict.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o test_secded speed_test_with_restrict speed_test_without_restrict
