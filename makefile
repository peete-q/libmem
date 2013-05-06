
CC = gcc
CFLAGS = -g
LFLAGS = -static-libgcc -Wl,-subsystem,console
TARGET = libmem.exe

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
all : $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LFLAGS)

clean :
	rm -f $(TARGET) $(OBJ)