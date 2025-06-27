CC := gcc
CFLAGS := -Wall -O0
TARGET := test5

.PHONY: all clean test_5

all: $(TARGET)

$(TARGET): test5.c
	$(CC) $(CFLAGS) $< -o $@

test_5: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 