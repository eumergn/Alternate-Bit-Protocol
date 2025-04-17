# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

# Executables
BINARIES = sender medium receiver

# Default target
all: $(BINARIES)

# Building
sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

medium: medium.c
	$(CC) $(CFLAGS) -o medium medium.c

receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c

# Clean target to remove compiled binaries
clean:
	rm -f $(BINARIES)
