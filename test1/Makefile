CC := gcc
CFLAGS := -Wall -O0
TARGET := test1

.PHONY: all clean test_1

all: $(TARGET)

$(TARGET): test1.c
	$(CC) $(CFLAGS) $< -o $@

test_1: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 