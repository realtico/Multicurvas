CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./include -lm
SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/multicurvas

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/*.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
