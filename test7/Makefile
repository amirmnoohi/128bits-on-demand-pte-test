CC := gcc
CFLAGS := -Wall -O0
TARGET := test7_program

.PHONY: all clean test_7

all: $(TARGET)

$(TARGET): test7.c
	$(CC) $(CFLAGS) $< -o $@

test_7: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 