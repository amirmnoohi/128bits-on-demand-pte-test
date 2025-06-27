CC := gcc
CFLAGS := -Wall -O0
TARGET := test3

.PHONY: all clean test_3

all: $(TARGET)

$(TARGET): test3.c
	$(CC) $(CFLAGS) $< -o $@

test_3: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 