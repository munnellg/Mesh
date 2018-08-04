OBJS = $(wildcard src/*.c)

CC = clang

CFLAGS = -Wall -Wpedantic -Wextra

LDFLAGS = -lSDL2 -lm

TARGET = mesh

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
