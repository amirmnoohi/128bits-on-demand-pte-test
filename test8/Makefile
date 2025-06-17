CC := gcc
CFLAGS := -Wall -O0
TARGET := test8_program

.PHONY: all clean test_8

all: $(TARGET)

$(TARGET): test8.c
	$(CC) $(CFLAGS) $< -o $@

test_8: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 