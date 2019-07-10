project('passgen', 'c', default_options : ['c_std=c11'])

passgen = library('passgen', 'src/passgen.c', 'src/pattern.c', 'src/random.c', 'src/reader.c')

utf8proc = meson.get_compiler('c').find_library('utf8proc')

# main executable
executable('passgen', 'src/bin/passgen.c', link_with : passgen)

# tools
executable('pattern-dump', 'src/bin/pattern-dump.c', link_with: passgen)
executable('pattern-choices', 'src/bin/pattern-choices.c', link_with: passgen)

# benchmarks
bench_random = executable('bench_random', 'src/bench/random.c', link_with: passgen)
benchmark('random', bench_random)

# tests
test('random', executable('test_random', 'src/tests/random.c', 'util/tests.c', link_with: passgen))
test('pattern', executable('test_pattern', 'src/tests/pattern.c', 'util/tests.c', link_with: passgen))

# man pages
install_man('man/passgen.1')

# headers
install_headers('passgen/random.h', 'passgen/passgen.h', 'passgen/pattern.h', subdir : 'passgen')
