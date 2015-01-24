CC:=cc

CFLAGS := -std=c99 -pedantic -Wall -Wno-parentheses -D_BSD_SOURCE \
	-D_GNU_SOURCE $(CFLAGS)
LDFLAGS := -lao -lsndfile $(LDFLAGS)

all: noisy

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

noisy: noisy.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f noisy
