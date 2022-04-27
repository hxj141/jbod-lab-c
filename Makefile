CC=gcc
CFLAGS=-c -Wall -I. -fpic -g -fbounds-check -Werror
LDFLAGS=-L.
LIBS=-lcrypto
SOURCES=tester.c util.c net.c 

OBJS=tester.o util.o mdadm.o cache.o net.o

tester:	$(OBJS) jbod.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

tester.o: tester.c
	$(CC) $(CFLAGS) $< -o $@

net.o: net.c
	$(CC) $(CFLAGS) $< -o $@

util.o: util.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f tester.o util.o net.o tester
 