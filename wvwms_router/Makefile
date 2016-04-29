CC=gcc
CFLAGS=-Wall -ggdb
DEPS=mspcom.h wvwms_types.h wvwms_display.h udp_network.h ini.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

wvwms_router: wvwms_display.o udp_network.o ini.o wvwms_router.o 
	gcc -o wvwms_router wvwms_display.o udp_network.o ini.o wvwms_router.o -I. -lm
