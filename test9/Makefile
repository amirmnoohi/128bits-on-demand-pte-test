CC := gcc
CFLAGS := -Wall -O0
TARGET := test9_program

.PHONY: all clean test_9

all: $(TARGET)

$(TARGET): test9.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f $(TARGET)

test_9: $(TARGET)
	@./$(TARGET) 