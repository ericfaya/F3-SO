CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lpthread

# Object files
FRAME_O = Libraries/frame.o
DIRFUNC_O = Libraries/dirfunctions.o
MD5FUNC_O = Libraries/md5functions.o
SEM_O = Libraries/semaphore_v2.o

# Define all targets
all: bowman poole discovery

$(SEM_O): Libraries/semaphore_v2.c Libraries/semaphore_v2.h
	$(CC) $(CFLAGS) -c $< -o $@ -I Libraries

# Frame object file compilation
$(FRAME_O): Libraries/frame.c Libraries/frame.h
	$(CC) $(CFLAGS) -c $< -o $@ -I Libraries

# dirfunctions object file compilation
$(DIRFUNC_O): Libraries/dirfunctions.c Libraries/dirfunctions.h
	$(CC) $(CFLAGS) -c $< -o $@ -I Libraries

# md5functions object file compilation
$(MD5FUNC_O): Libraries/md5functions.c Libraries/md5functions.h
	$(CC) $(CFLAGS) -c $< -o $@ -I Libraries

# Bowman compilation
bowman: $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) $(SEM_O) Bowman/Bowman.c Bowman/config.c
	$(CC) $(CFLAGS) -o $@ Bowman/Bowman.c Bowman/config.c $(FRAME_O) $(DIRFUNC_O) $(SEM_O) $(MD5FUNC_O) $(LIBS) -I Bowman -I Libraries

# Poole compilation
poole: $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) Poole/Poole.c Poole/config.c
	$(CC) $(CFLAGS) -o $@ Poole/Poole.c Poole/config.c $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) $(LIBS) -I Poole -I Libraries

# Discovery compilation
discovery: $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) Discovery/Discovery.c Discovery/PooleList.c Discovery/config.c
	$(CC) $(CFLAGS) -o $@ Discovery/Discovery.c Discovery/PooleList.c Discovery/config.c $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) $(LIBS) -I Discovery -I Libraries

# Clean compiled files and executables
clean:
	rm -f bowman poole discovery $(FRAME_O) $(DIRFUNC_O) $(MD5FUNC_O) $(SEM_O)
