# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
FORMAT       = clang-format -i
MESON        = meson
NINJA        = ninja

default: release

# build and run tests.
test: debug
	ninja -C build/debug test

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
