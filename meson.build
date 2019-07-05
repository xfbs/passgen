project('passgen', 'c', default_options : ['c_std=c11'])

passgen = library('passgen', 'src/passgen.c', 'src/pattern.c', 'src/random.c')

executable('passgen', 'src/bin/passgen.c', link_with : passgen)

executable('bench_random', 'src/bench/random.c', link_with: passgen)

test('random_tests', executable('random_tests', 'src/tests/random.c', 'util/tests.c', link_with: passgen))
test('pattern_tests', executable('pattern_tests', 'src/tests/pattern.c', 'util/tests.c', link_with: passgen))
