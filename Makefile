# Makefile with common tasks for this repo
#
# This is provided for convenience, but it is recommended that you use CMake
# directly, and for better compilation speed it is recommended to use Ninja
# (package `ninja-build` in Debian) rather than Make.
DOCKER=docker
DOCKER_IMAGE=registry.gitlab.com/xfbs/passgen
DOXYGEN=doxygen
BUILDDIR=build
BUILD_TYPE=Debug

# build passgen
passgen: $(BUILDDIR)/src/cli/passgen

# run benchmark
bench: $(BUILDDIR)/src/bench/passgen-bench
	@./$<

# run unit tests
test: $(BUILDDIR)/src/tests/passgen-test
	@./$< -v

# compute coverage
coverage:
	@./scripts/coverage.sh build-coverage

# run AddressSanitizer against the code
asan:
	@./scripts/sanitizer.sh Address

# run MemorySanitizer against the code, may have to set `CC=clang`.
msan:
	@./scripts/sanitizer.sh Memory

# run UndefinedSanitizer agains the code
usan:
	@./scripts/sanitizer.sh Undefined

# format source code
format: $(BUILDDIR)
	cd build && make clangformat

# build CLI and libraries
$(BUILDDIR)/src/cli/passgen: $(BUILDDIR)
	@cd build && make -j

# build benchmarks
$(BUILDDIR)/src/bench/passgen-bench: $(BUILDDIR)
	@cd build && make -j passgen-bench

# build tests
$(BUILDDIR)/src/tests/passgen-test: $(BUILDDIR)
	@cd build && make -j passgen-test

# create and initialize build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	@cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

# clean up build dir
clean:
	$(RM) -r $(BUILDDIR)

# build docker images
docker-build:
	$(DOCKER) build scripts -f scripts/riscv64.dockerfile -t $(DOCKER_IMAGE)/riscv64
	$(DOCKER) build scripts -f scripts/aarch64.dockerfile -t $(DOCKER_IMAGE)/aarch64
	$(DOCKER) build scripts -f scripts/mingw64.dockerfile -t $(DOCKER_IMAGE)/mingw64
	$(DOCKER) build scripts -f scripts/amd64.dockerfile -t $(DOCKER_IMAGE)/amd64

# build docs
docs:
	$(DOXYGEN) Doxyfile

.PHONY: docs bench test passgen format asan usan msan
