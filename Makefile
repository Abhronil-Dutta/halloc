CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -D_DEFAULT_SOURCE
TARGET = test
SRCS = halloc.c test.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)