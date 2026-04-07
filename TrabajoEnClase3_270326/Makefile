CC = gcc
CFLAGS = -Wall -Wextra -lpthread
TARGET = diningPhilosophers
SRC = diningPhilosophers.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET) *.out *.err

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run