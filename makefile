
CC = gcc
CFLAGS = -W -Wall -ggdb
LFLAGS = 
TARGET = libmem.a
TEST = test
AR = ar rcu

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
all : $(OBJ)
	$(AR) -o $(TARGET) $(OBJ) $(LFLAGS)
	$(CC) -o $(TEST) $(OBJ) $(LFLAGS)

clean :
	rm -f $(TARGET) $(OBJ)
