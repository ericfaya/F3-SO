CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lpthread

# Define all targets
all: frame.o bowman poole discovery

# Frame object file compilation
frame.o:
	$(CC) $(CFLAGS) -c Frame/Frame.c -o Frame/Frame.o -I Frame

# Bowman compilation
bowman: frame.o
	$(CC) $(CFLAGS) -o bowman Bowman/Bowman.c Bowman/config.c Frame/Frame.c $(LIBS) -I Bowman -I Frame

# Poole compilation
poole: frame.o
	$(CC) $(CFLAGS) -o poole Poole/Poole.c Poole/config.c Frame/Frame.c $(LIBS) -I Poole -I Frame

# Discovery compilation
discovery: frame.o
	$(CC) $(CFLAGS) -o discovery Discovery/Discovery.c Discovery/PooleList.c Discovery/config.c Frame/Frame.c $(LIBS) -I Discovery  -I Frame

# Clean compiled files and executables
clean:
	rm -f bowman poole discovery Frame/*.o
