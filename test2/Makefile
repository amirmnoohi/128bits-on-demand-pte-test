CC := gcc
CFLAGS := -Wall -O0
TARGET := test2

.PHONY: all clean test_2

all: $(TARGET)

$(TARGET): test2.c
	$(CC) $(CFLAGS) $< -o $@

test_2: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 