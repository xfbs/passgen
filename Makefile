CC			 = clang
RM			 = rm -f
AR			 = ar
MKDIR		 = mkdir -p
CFLAGS		+= -std=c99 -Wall -Wextra -pedantic -Iinclude
LDFLAGS		 =
SOURCES		 = random.c
LIBNAME      = passgen
BINARY		 = passgen.c
TESTS		 =

default: release

release: $(BINARY:%.c=build/release/%)

build/release:
	$(MKDIR) $@

build/release/%: build/release/%.o build/release/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/release/$(LIBNAME).a: $(SOURCES:%.c=build/release/%.o)
	ar rcs $@ $^

build/release/%.o: src/%.c build/release
	$(CC) -c $(CFLAGS) $< -o $@

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) -r build
