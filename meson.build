project('passgen', 'c', default_options : ['c_std=c11'])

passgen = library('passgen', 'src/passgen.c', 'src/pattern.c', 'src/random.c')

executable('passgen', 'src/bin/passgen.c', link_with : passgen)

# benchmarks
executable('bench_random', 'src/bench/random.c', link_with: passgen)

# tests
test('random', executable('test_random', 'src/tests/random.c', 'util/tests.c', link_with: passgen))
test('pattern', executable('test_pattern', 'src/tests/pattern.c', 'util/tests.c', link_with: passgen))
