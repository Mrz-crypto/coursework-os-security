CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -std=c11 -D_GNU_SOURCE

all: Frontend Backend

Frontend: Frontend.c
	$(CC) $(CFLAGS) -o Frontend Frontend.c

Backend: Backend.c
	$(CC) $(CFLAGS) -o Backend Backend.c

clean:
	rm -f Frontend Backend
