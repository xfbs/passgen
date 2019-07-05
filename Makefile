# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
FORMAT       = clang-format -i
MESON        = meson
NINJA        = ninja

default: release

# build and run tests.
test: debug
	build/debug/random_tests
	build/debug/pattern_tests

# build and run tests, with address-sanitizer enabled.
test-address: debug-address
	ninja -C build/debug-address test

bench: debug
	build/debug/bench_random

#all-debug: debug debug-memory debug-address debug-undefined

# format using clang-format.
format:
	$(FORMAT) passgen/*.h src/*.c src/bin/*.c

# targets.
release: build/release
	ninja -C $<

debug: build/debug
	ninja -C $<

debug-address: build/debug-address
	ninja -C $<

debug-memory: build/debug-memory
	ninja -C $<

build/release:
	meson $@

build/debug:
	meson $@ --buildtype=debug

build/debug-address:
	meson $@ --buildtype=debug -Db_sanitize=address,undefined

build/debug-memory:
	meson $@ --buildtype=debug -Db_sanitize=memory

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) -r build

.PHONY: format release debug install clean
