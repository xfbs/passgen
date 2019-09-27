# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
FORMAT       = clang-format -i
MESON        = meson
NINJA        = ninja
DOXYGEN      = doxygen
DESTDIR		 = /
PREFIX		 = /usr

default: release

# install locally.
install: release
	cd build/release && meson install

# generate debian package.
deb: DEBNAME=$(NAME)-$(VERSION)
deb: CONTROL=build/release/$(NAME)-$(VERSION)/DEBIAN/control
deb: release
	mkdir -p build/release/$(DEBNAME)/DEBIAN
	echo "Package: passgen" > $(CONTROL)
	echo "Version: $(VERSION)" >> $(CONTROL)
	echo "Section: custom" >> $(CONTROL)
	echo "Priority: optional" >> $(CONTROL)
	echo "Architecture: all" >> $(CONTROL)
	echo "Essential: no" >> $(CONTROL)
	echo "Installed-Size: 1024" >> $(CONTROL)
	echo "Maintainer: Patrick M. Elsen" >> $(CONTROL)
	echo "Description: Generate passwords using a regex-like pattern." >> $(CONTROL)
	cd build/release && DESTDIR=$(DEBNAME) meson install --prefix=/usr
	cd build/release && dpkg-deb --build $(DEBNAME)

# generate documentations using doxygen.
docs:
	$(DOXYGEN) Doxyfile

# build and run tests.
test: debug
	build/debug/tests

# build and run tests with address sanitizer.
test-address: debug-address
	build/debug-address/test_random
	build/debug-address/test_pattern
	build/debug-address/test_reader

# build and run benchmarks.
bench: debug
	build/debug/bench_random

# format using clang-format.
format:
	$(FORMAT) passgen/*.h tests/*.c src/*.c src/bin/*.c src/bench/*.c

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
	meson --prefix=$(PREFIX) $@ -Db_ndebug=true

build/debug:
	meson $@ --buildtype=debug

build/debug-address:
	meson $@ --buildtype=debug -Db_sanitize=address,undefined

build/debug-memory:
	meson $@ --buildtype=debug -Db_sanitize=memory

clean:
	$(RM) -r build docs/html

.PHONY: format release debug install clean docs
