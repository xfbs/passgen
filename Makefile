CC=gcc
RM=rm -f
CFLAGS+=-std=c99 -Wall -pedantic
LDFLAGS=-ggdb3
SOURCES=havege.c timing.c passgen.c
TARGET=passgen

.DEFAULT: $(TARGET)
.SUFFIXES: .c.o
.c.o: 
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $(TARGET) $^

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) $(SOURCES:.c=.o)
