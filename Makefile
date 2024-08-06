CC = gcc
CFLAGS = -I./include
LDFLAGS = -L./lib -lSDL2 -lSDL2main

SRC = chip8.c
OBJ = $(SRC:.c=.o)
TARGET = chip8.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)