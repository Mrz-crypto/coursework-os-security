CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -std=c11

all: Frontend Backend

Frontend: Frontend.c
	$(CC) $(CFLAGS) -o Frontend Frontend.c

Backend: Backend.c
	$(CC) $(CFLAGS) -o Backend Backend.c

clean:
	rm -f Frontend Backend
