CC:=cc

CFLAGS := -std=c99 -pedantic -Wall -Wno-parentheses -D_BSD_SOURCE \
	-D_GNU_SOURCE $(CFLAGS)
LDFLAGS := $(LDFLAGS)

all: refresher

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

refresher: refresher.o gpio.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f refresher
