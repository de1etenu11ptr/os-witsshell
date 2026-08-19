CC=gcc
CFLAGS=-Wall -Werror -g

ifdef DEBUG
ifneq ($(DEBUG), 0)
	CFLAGS+= -DDEBUG
endif
endif

TARGET=witsshell

SOURCES=$(shell find ./ -type f -iname "*.c")
OBJECTS=$(patsubst ./%.c, ./%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $?

clean:
	rm -rf $(TARGET) $(OBJECTS)

