# passgen build description.
project(
  'passgen',
  'c',
  default_options: ['c_std=c11'],
  version: '0.1.2',
  license: 'MIT')

# create man page
conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('VERSION_MAJ', meson.project_version().split('.')[0])
conf_data.set('VERSION_MIN', meson.project_version().split('.')[1])
conf_data.set('VERSION_REV', meson.project_version().split('.')[2])
configure_file(
  input: 'man/passgen.1.in',
  output: 'passgen.1',
  configuration: conf_data,
  install_dir: 'share/man/man1/')
version_src = configure_file(
  input: 'src/version.c.in',
  output: 'version.c',
  configuration: conf_data)

cc = meson.get_compiler('c')

# library used to decode and encode utf8.
libutf8proc = dependency('libutf8proc', required: false, static: get_option('static'))

if not libutf8proc.found()
  libutf8proc = cc.find_library('utf8proc', static: get_option('static'))
endif

# library used for cryptographic routines.
libsodium = dependency('libsodium', static: get_option('static'))

# library used for generating JSON.
libjansson = dependency('jansson', static: get_option('static'))

# math lib
libm = cc.find_library('m')

# threads
threads = dependency('threads')

deps = [libutf8proc]

# main library
libpassgen_src = [
  'src/passgen.c',
  'src/pattern.c',
  'src/pattern_random.c',
  'src/random.c',
  'src/reader.c',
  'src/unicode.c',
  'src/token.c',
  'src/memory.c',
  'src/array.c',
  'src/pronounceable.c',
  'data/pronounceable_english.c',
  'data/pronounceable_latin.c',
  'data/pattern_presets.c',
  'src/assert.c',
]

if not get_option('static')
libpassgen = library(
  'passgen',
  libpassgen_src,
  version_src,
  install: true,
  dependencies: deps)
else
libpassgen = static_library(
  'passgen',
  libpassgen_src,
  version_src,
  install: true,
  dependencies: deps)
endif

# main executable
passgen = executable(
  'passgen',
  'src/bin/passgen.c',
  link_with: libpassgen,
  install: true)

# tools
#pattern_dump = executable(
#  'pattern-dump',
#  'src/bin/pattern-dump.c',
#  link_with: libpassgen)
#pattern_choices = executable(
#  'pattern-choices',
#  'src/bin/pattern-choices.c',
#  link_with: libpassgen)

# benchmarks
bench_random = executable(
  'bench_random',
  'src/bench/random.c',
  link_with: libpassgen)
benchmark('random', bench_random)
bench_parse = executable(
  'bench_parse',
  'src/bench/parse.c',
  'src/bench/bench.c',
  'src/bench/all.c',
  link_with: libpassgen,
  dependencies: [libjansson, libm, threads])
benchmark('parse', bench_parse)

# tests
generate_all = find_program('tests/generate.rb')
test_files = [
  'tests/random.c',
  'tests/memory.c',
  'tests/reader.c',
  'tests/pattern.c',
  'tests/pattern_random.c',
  'tests/unicode.c',
  'tests/array.c',
  'tests/pronounceable.c',
  'tests/token.c',
]
all_tests = custom_target(
  'all.c',
  input: test_files,
  output: 'all.c',
  command: [generate_all, '@OUTPUT@', '@INPUT@'])
tests = executable(
  'tests',
  'tests/tests.c',
  test_files,
  all_tests,
  link_with: libpassgen)
test('tests', tests)

# headers
install_headers(
  'passgen/random.h',
  'passgen/passgen.h',
  'passgen/pattern.h',
  'passgen/version.h',
  'passgen/unicode.h',
  'passgen/token.h',
  subdir : 'passgen')

# pkgconfig file
pkg = import('pkgconfig')
pkg.generate(
  libpassgen,
  description: 'Regex-based password generation')
