# global variables. we target at compilation with clang.
# all warnings enabled by defaul. use modern C standard.
FORMAT       = clang-format -i
MESON        = meson
NINJA        = ninja
DOXYGEN      = doxygen
NAME         = passgen
VERSION      = 1.0.0

default: release

# install locally.
install: release
	cd build/release && meson install
	gzip /usr/local/share/man/man1/passgen.1

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
	cd build/release && DESTDIR=$(DEBNAME) meson install
	cd build/release && dpkg-deb --build $(DEBNAME)

# generate documentations using doxygen.
docs:
	$(DOXYGEN) Doxyfile

# build and run tests.
test: debug
	build/debug/test_random
	build/debug/test_pattern

# build and run tests with address sanitizer.
test-address: debug-address
	build/debug-address/test_random
	build/debug-address/test_pattern

# build and run benchmarks.
bench: debug
	build/debug/bench_random

# format using clang-format.
format:
	$(FORMAT) passgen/*.h util/*.c util/*.h src/*.c src/bin/*.c

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

clean:
	$(RM) -r build docs

.PHONY: format release debug install clean docs
