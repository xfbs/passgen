project(
  'passgen',
  'c',
  default_options: ['c_std=c11'],
  version: '0.1.0',
  license: 'MIT')

# create man page
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(
  input: 'man/passgen.1.in',
  output: 'passgen.1',
  configuration: conf_data,
  install_dir: 'share/man/man1/')

# main library
passgen = library('passgen', 'src/passgen.c', 'src/pattern.c', 'src/random.c', 'src/reader.c', 'src/unicode.c')

# library used to decode and encode utf8
utf8proc = meson.get_compiler('c').find_library('utf8proc')

# library used for cryptographic routines
libsodium = meson.get_compiler('c').find_library('sodium')

# main executable
executable('passgen', 'src/bin/passgen.c', link_with : passgen)

# tools
executable('pattern-dump', 'src/bin/pattern-dump.c', link_with: passgen)
executable('pattern-choices', 'src/bin/pattern-choices.c', link_with: passgen)

# benchmarks
bench_random = executable('bench_random', 'src/bench/random.c', link_with: passgen)
benchmark('random', bench_random)

# tests
test_random = executable('test_random', 'tests/random.c', 'tests/tests.c', link_with: passgen)
test_pattern = executable('test_pattern', 'tests/pattern.c', 'tests/tests.c', link_with: passgen)
test_reader = executable('test_reader', 'tests/reader.c', 'tests/tests.c', link_with: passgen)
test('random', test_random)
test('pattern', test_pattern)
test('reader', test_reader)

# man pages
#install_man('passgen.1')

# headers
install_headers('passgen/random.h', 'passgen/passgen.h', 'passgen/pattern.h', subdir : 'passgen')

# pkgconfig gile
pkg = import('pkgconfig')
pkg.generate(passgen, description: 'Regex-based password generation')
