# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
CC			 = clang
RM			 = rm -f
AR			 = ar
STRIP		 = strip
MKDIR		 = mkdir -p
CFLAGS		+= -std=c99 -Wall -Wextra -pedantic -Iinclude
LDFLAGS		 =
SOURCES		 = random.c
LIBNAME      = passgen
BINARY		 = passgen.c
TESTS		 =

default: release

release: CFLAGS  += -O3
release: LDFLAGS += -O3
release: $(BINARY:%.c=build/release/%)

# release build
# enables optimizations
build/release:
	$(MKDIR) $@

build/release/%: build/release/%.o build/release/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^
	$(STRIP) $@

build/release/$(LIBNAME).a: $(SOURCES:%.c=build/release/%.o)
	$(AR) rcs $@ $^

build/release/%.o: src/%.c build/release
	$(CC) -c $(CFLAGS) $< -o $@

# debug build
# enables debug symbols, defines debug.
debug: CFLAGS  += -O1 -g -DDEBUG
debug: LDFLAGS += -O1 -g
debug: $(BINARY:%.c=build/debug/%)

build/debug:
	$(MKDIR) $@

build/debug/%: build/debug/%.o build/debug/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/debug/$(LIBNAME).a: $(SOURCES:%.c=build/debug/%.o)
	$(AR) rcs $@ $^

build/debug/%.o: src/%.c build/debug
	$(CC) -c $(CFLAGS) $< -o $@

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) -r build
