project('passgen', 'c')
passgen = library('passgen', 'src/passgen.c', 'src/pattern.c', 'src/random.c')
executable('passgen', 'src/bin/passgen.c', link_with : passgen)
executable('tests', 'src/bin/tests.c', link_with: passgen)
