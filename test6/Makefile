CC := gcc
CFLAGS := -Wall -O0
TARGET := test6_program

.PHONY: all clean test_6

all: $(TARGET)

$(TARGET): test6.c
	$(CC) $(CFLAGS) $< -o $@

test_6: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(TARGET) 