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
utf8proc = dependency('libutf8proc')

# library used for cryptographic routines
libsodium = dependency('libsodium')

# main executable
executable('passgen', 'src/bin/passgen.c', link_with : passgen)

# tools
executable('pattern-dump', 'src/bin/pattern-dump.c', link_with: passgen)
executable('pattern-choices', 'src/bin/pattern-choices.c', link_with: passgen)

# benchmarks
bench_random = executable('bench_random', 'src/bench/random.c', link_with: passgen)
benchmark('random', bench_random)

ruby = find_program('tests/generate.rb')

test_files = ['tests/random.c', 'tests/reader.c', 'tests/pattern.c']

all_tests = custom_target(
  'all.c',
  input: test_files,
  output: 'all.c',
  command: [ruby, '@OUTPUT@', '@INPUT@'])

# tests
tests = executable('tests', 'tests/tests.c', test_files, all_tests, link_with: passgen)
test('tests', tests)

# man pages
#install_man('passgen.1')

# headers
install_headers('passgen/random.h', 'passgen/passgen.h', 'passgen/pattern.h', subdir : 'passgen')

# pkgconfig gile
pkg = import('pkgconfig')
pkg.generate(passgen, description: 'Regex-based password generation')
