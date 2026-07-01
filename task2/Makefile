CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -std=c11 -D_GNU_SOURCE
PTHREAD=-pthread

TEST_DIR=test-binaries

all: Sandbox safe_program infinite_loop cpu_hog slow_program

Sandbox: Sandbox.c
	$(CC) $(CFLAGS) $(PTHREAD) -o Sandbox Sandbox.c

safe_program: $(TEST_DIR)/safe_program.c
	$(CC) $(CFLAGS) -o $(TEST_DIR)/safe_program $(TEST_DIR)/safe_program.c

infinite_loop: $(TEST_DIR)/infinite_loop.c
	$(CC) $(CFLAGS) -o $(TEST_DIR)/infinite_loop $(TEST_DIR)/infinite_loop.c

cpu_hog: $(TEST_DIR)/cpu_hog.c
	$(CC) $(CFLAGS) -o $(TEST_DIR)/cpu_hog $(TEST_DIR)/cpu_hog.c

slow_program: $(TEST_DIR)/slow_program.c
	$(CC) $(CFLAGS) -o $(TEST_DIR)/slow_program $(TEST_DIR)/slow_program.c

clean:
	rm -f Sandbox $(TEST_DIR)/safe_program $(TEST_DIR)/infinite_loop $(TEST_DIR)/cpu_hog $(TEST_DIR)/slow_program
