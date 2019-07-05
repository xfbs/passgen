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

default: release

# run tests.
tests: debug
	build/debug/tests

all-debug: debug debug-memory debug-address debug-undefined

all-tests: all-debug
	build/debug-memory/tests
	build/debug-address/tests
	build/debug-undefined/tests

# format using clang-format.
format:
	$(FORMAT) $(HEADERS:%.h=include/%.h) $(SOURCES:%.c=src/%.c) $(BINARY:%.c=src/%.c)

# release build
# enables optimizations
release: CFLAGS  += -O3
release: LDFLAGS += -O3
release: $(BINARY:%.c=build/release/%)

build/release:
	$(MKDIR) $@ $@/obj-bin $@/obj-lib

build/release/%: build/release/obj-bin/%.o build/release/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^
	$(STRIP) $@

build/release/$(LIBNAME).a: $(SOURCES:%.c=build/release/obj-lib/%.o)
	$(AR) rcs $@ $^

build/release/obj-bin/%.o: src/bin/%.c build/release
	$(CC) -c $(CFLAGS) $< -o $@

build/release/obj-lib/%.o: src/%.c build/release
	$(CC) -c $(CFLAGS) $< -o $@

# debug build
# enables optimizations
debug: CFLAGS  += -O1 -g -DDEBUG
debug: LDFLAGS += -O1 -g
debug: $(BINARY:%.c=build/debug/%)

build/debug:
	$(MKDIR) $@ $@/obj-bin $@/obj-lib

build/debug/%: build/debug/obj-bin/%.o build/debug/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/debug/$(LIBNAME).a: $(SOURCES:%.c=build/debug/obj-lib/%.o)
	$(AR) rcs $@ $^

build/debug/obj-bin/%.o: src/bin/%.c build/debug
	$(CC) -c $(CFLAGS) $< -o $@

build/debug/obj-lib/%.o: src/%.c build/debug
	$(CC) -c $(CFLAGS) $< -o $@

# debug-memory build with memory sanitizer
# enables optimizations
debug-memory: CFLAGS  += -O1 -g -DDEBUG -fsanitize=memory
debug-memory: LDFLAGS += -O1 -g -fsanitize=memory
debug-memory: $(BINARY:%.c=build/debug-memory/%)

build/debug-memory:
	$(MKDIR) $@ $@/obj-bin $@/obj-lib

build/debug-memory/%: build/debug-memory/obj-bin/%.o build/debug-memory/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/debug-memory/$(LIBNAME).a: $(SOURCES:%.c=build/debug-memory/obj-lib/%.o)
	$(AR) rcs $@ $^

build/debug-memory/obj-bin/%.o: src/bin/%.c build/debug-memory
	$(CC) -c $(CFLAGS) $< -o $@

build/debug-memory/obj-lib/%.o: src/%.c build/debug-memory
	$(CC) -c $(CFLAGS) $< -o $@

# debug-address build with address sanitizer (and leak sanitizer)
# enables optimizations
debug-address: CFLAGS  += -O1 -g -DDEBUG -fsanitize=address
debug-address: LDFLAGS += -O1 -g -fsanitize=address
debug-address: $(BINARY:%.c=build/debug-address/%)

build/debug-address:
	$(MKDIR) $@ $@/obj-bin $@/obj-lib

build/debug-address/%: build/debug-address/obj-bin/%.o build/debug-address/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/debug-address/$(LIBNAME).a: $(SOURCES:%.c=build/debug-address/obj-lib/%.o)
	$(AR) rcs $@ $^

build/debug-address/obj-bin/%.o: src/bin/%.c build/debug-address
	$(CC) -c $(CFLAGS) $< -o $@

build/debug-address/obj-lib/%.o: src/%.c build/debug-address
	$(CC) -c $(CFLAGS) $< -o $@

# debug-undefined build with leak sanitizer
# enables optimizations
debug-undefined: CFLAGS  += -O1 -g -DDEBUG -fsanitize=undefined
debug-undefined: LDFLAGS += -O1 -g -fsanitize=undefined
debug-undefined: $(BINARY:%.c=build/debug-undefined/%)

build/debug-undefined:
	$(MKDIR) $@ $@/obj-bin $@/obj-lib

build/debug-undefined/%: build/debug-undefined/obj-bin/%.o build/debug-undefined/$(LIBNAME).a
	$(CC) -o $@ $(LDFLAGS) $^

build/debug-undefined/$(LIBNAME).a: $(SOURCES:%.c=build/debug-undefined/obj-lib/%.o)
	$(AR) rcs $@ $^

build/debug-undefined/obj-bin/%.o: src/bin/%.c build/debug-undefined
	$(CC) -c $(CFLAGS) $< -o $@

build/debug-undefined/obj-lib/%.o: src/%.c build/debug-undefined
	$(CC) -c $(CFLAGS) $< -o $@

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	$(RM) -r build

.PHONY: format release debug install clean
