CC=gcc
RM=rm -f
CFLAGS+=-std=c99
LDFLAGS=-ggdb3
SOURCES=havege.c timing.c passgen.c
TARGET=passgen

.SUFFIXES: .c.o
.c.o: 
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $(TARGET) $^

clean:
	$(RM) $(SOURCES:.c=.o)
