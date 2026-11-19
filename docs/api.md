# libpassgen

libpassgen generates passwords and passphrases from patterns, such as
`[a-z]{5}(-[a-z]{5}){3}` or `\w{english}(-\w{english}){4}`. It is what the
`passgen` command-line tool is built on. The syntax of patterns is described
on [passgen.io](https://passgen.io).

This is the documentation of its C interface. It is written in C99, and can be
used from C++ as well.

## Using it

Include `passgen/passgen.h`, which has all of the interface, and link against
`libpassgen`:

```c
#include <passgen/passgen.h>
```

pkg-config knows about it, as `passgen`:

```sh
cc -o program program.c $(pkg-config --cflags --libs passgen)
```

Generating takes three things:

1. **A pattern,** which is parsed from a string with @ref passgen_parse.
2. **A source of randomness,** a @ref passgen_random. For passwords, this is
   the one of the system (@ref passgen_random_system_open). Master passphrase
   mode derives one from a passphrase instead
   (@ref passgen_random_chacha20_argon2_open).
3. **An environment,** a @ref passgen_env, which has the source of randomness
   and the wordlists which patterns can use.

Before generating, the pattern is resolved with the environment
(@ref passgen_resolve). This reads the wordlists which the pattern uses, and
fails if one is missing, so that problems come up before anything is
generated. After that, one of the functions in @ref generate.h generates from
the pattern, into a buffer or one codepoint at a time, as often as needed.

## Example

This program generates from a pattern, with a wordlist if one is given. It is
built and run along with the tests, so it works with the version of the
library that this documentation is for.

@include generate.c

## Conventions

### Who owns what

- What a function allocates is freed by the function of the same type which
  ends in `_free`: @ref passgen_parse allocates a pattern, which
  @ref passgen_pattern_free frees, and @ref passgen_env_new allocates an
  environment, which @ref passgen_env_free frees. These do nothing when given
  NULL.
- Sources of randomness can be opened in place, or allocated, and are closed
  or freed accordingly. See @ref passgen_random.
- An environment does not own its source of randomness, which has to stay
  open for as long as the environment is used. It owns the files of its
  wordlists, and copies their names.
- Functions do not hold on to strings or buffers which they are given, apart
  from the files of wordlists.
- A resolved pattern refers to the wordlists of its environment, so it can
  only be generated with that environment. Adding a wordlist to it means
  resolving the pattern again.

### Errors

- Functions which can fail return a @ref passgen_status, which is
  @ref PASSGEN_OK (zero) when nothing went wrong. Those which allocate return
  NULL instead.
- Those which can say more take a @ref passgen_error, which they fill in when
  they fail, and which then has to be released with @ref passgen_error_free.
  It may be NULL. See @ref error.h.
- Running out of memory is an error like any other
  (@ref PASSGEN_ERROR_OUT_OF_MEMORY), and leaves nothing allocated.
- If generating fails, what was output so far has to be thrown away.
- The library only aborts when it is used in a way which is not valid and
  there is no way to return an error, such as asking
  @ref passgen_random_u32_max for a number smaller than zero. Debug builds
  (see @ref passgen_is_debug) also stop when something which should not be
  possible happens.

### Threads

- Patterns are only read when generating, so one pattern can be used by many
  threads at once.
- Environments and sources of randomness are changed when they are used, so
  each one may only be used by one thread at a time. Separate ones can be
  used by separate threads.
- The only state which is shared by everything is the allocator
  (see @ref allocator.h), which is replaced before the library is used, if at
  all.

### Secrets

- Sources of randomness clear their state when they are closed, and keys
  derived from master passphrases are wiped once they are not needed.
- What is generated is in buffers which belong to the caller, who has to
  clear them once they are not needed.
- Memory which holds secrets can be kept out of swap with an allocator which
  locks it (see @ref passgen_allocator_set). The command-line tool locks all
  of its memory instead.

### Versions

The version of the headers is in @ref config.h.in "passgen/config.h", and the
version of the library which is used at runtime comes from
@ref passgen_version_get. Before 1.0.0, every minor version may change the
interface. What master passphrase mode generates never changes.

## Headers

| Header | What it has |
| --- | --- |
| @ref passgen.h | Parsing patterns, and all of the other headers |
| @ref generate.h | Generating from a pattern |
| @ref env.h | Environments and wordlists |
| @ref random.h | Sources of randomness |
| @ref error.h | Errors |
| @ref allocator.h | Replacing the allocator |
| @ref version.h | The version of the library |
| @ref config.h.in "config.h" | The version of the headers, and how the library was built |
