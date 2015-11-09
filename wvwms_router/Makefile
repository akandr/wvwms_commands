CC=gcc
CFLAGS=-Wall
DEPS=mspcom.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

wvwms_router: wvwms_router.o
	gcc -o wvwms_router wvwms_router.o -I.
