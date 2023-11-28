CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lpthread

# Define all targets
all: bowman poole discovery

# Frame object file compilation
frame.o: Frame/Frame.c Frame/Frame.h
	$(CC) $(CFLAGS) -c Frame/Frame.c -o Frame/Frame.o -I Frame

# Bowman compilation
bowman: frame.o Bowman/Bowman.c Bowman/config.c
	$(CC) $(CFLAGS) -o bowman Bowman/Bowman.c Bowman/config.c Frame/Frame.o $(LIBS) -I Bowman -I Frame

# Poole compilation
poole: frame.o Poole/Poole.c Poole/config.c
	$(CC) $(CFLAGS) -o poole Poole/Poole.c Poole/config.c Frame/Frame.o $(LIBS) -I Poole -I Frame

# Discovery compilation
discovery: frame.o Discovery/Discovery.c Discovery/PooleList.c Discovery/config.c
	$(CC) $(CFLAGS) -o discovery Discovery/Discovery.c Discovery/PooleList.c Discovery/config.c Frame/Frame.o $(LIBS) -I Discovery  -I Frame

# Clean compiled files and executables
clean:
	rm -f bowman poole discovery Frame/*.o
