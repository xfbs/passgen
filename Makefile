# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
CC			 = clang
RM			 = rm -f
AR			 = ar
STRIP		 = strip
MKDIR		 = mkdir -p
FORMAT       = clang-format -i
CFLAGS		+= -std=c99 -Wall -Wextra -pedantic -I.
LDFLAGS		 =
SOURCES		 = random.c pattern.c passgen.c
HEADERS      = random.h pattern.h
LIBNAME      = passgen
BINARY		 = tests.c passgen.c
TESTS		 =
MESON        = meson
NINJA        = ninja

default: release

# run tests.
tests: debug
	build/debug/tests

#all-debug: debug debug-memory debug-address debug-undefined

# format using clang-format.
format:
	$(FORMAT) passgen/*.h src/*.c src/bin/*.c

# targets.
release: build/release
	ninja -C build/release

build/release:
	meson build/release

debug: build/debug
	ninja -C build/debug

build/debug:
	meson build/debug --buildtype=debug

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) -r build

.PHONY: format release debug install clean
