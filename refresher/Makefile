CC:=cc

CFLAGS := -std=c99 -pedantic -Wall -Wno-parentheses -D_BSD_SOURCE $(CFLAGS)
LDFLAGS := $(LDFLAGS)

all: refresher

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

refresher: refresher.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f refresher
