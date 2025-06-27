CC := gcc
CFLAGS := -Wall -O0
TARGET := test4

.PHONY: all clean test_4

all: $(TARGET)

$(TARGET): test4.c
	$(CC) $(CFLAGS) $< -o $@

test_4: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 