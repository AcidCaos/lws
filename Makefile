CC := gcc
CFLAGS := -O2 -Wall -Wextra -Werror -std=c11

all: lws

lws: lws.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run:
	./lws 8080
