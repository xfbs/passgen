#include "passgen.h"

#ifdef PASSGEN_MLOCK
#include <sys/mman.h>
#include <sys/resource.h>

/// How much memory we need to be allowed to lock before we ask for all future
/// allocations to be locked.
#define PASSGEN_MLOCK_FUTURE_MIN ((rlim_t) 64 * 1024 * 1024)

/// Locks the memory used by passgen to avoid it being swapped to disk, which
/// would leak secret state, such as the generated output or the randomness
/// source buffer.
///
/// This is best-effort. The memory that is in use right now is always locked,
/// if possible. Future allocations (MCL_FUTURE) are only locked if the limit
/// for locked memory is high enough: once that is enabled, any allocation
/// which would exceed the limit fails, and the limit is commonly lower than
/// what is needed to load a wordlist or to derive a key from a master
/// passphrase.
static void passgen_cli_lock_memory(void) {
    int flags = MCL_CURRENT;

    struct rlimit limit;
    if(0 == getrlimit(RLIMIT_MEMLOCK, &limit)) {
        if(limit.rlim_cur == RLIM_INFINITY ||
           limit.rlim_cur >= PASSGEN_MLOCK_FUTURE_MIN) {
            flags |= MCL_FUTURE;
        }
    }

    // if this fails (which it does when there is already more memory in use
    // than may be locked), nothing is locked, and there is nothing we can do
    // about it.
    (void) mlockall(flags);
}
#endif

int main(int argc, char *argv[]) {
    int ret;
    passgen_cli_opts opts;

#ifdef PASSGEN_MLOCK
    passgen_cli_lock_memory();
#endif

    // seccomp is a linux feature that allows us to load filters for the
    // syscalls we are allowed to issue. this means that even if there is a
    // memory unsafety bug that allows arbitrary code execution, this process is
    // still sandboxed and can only perform safe system calls.
#ifdef PASSGEN_SECCOMP
    passgen_cli_seccomp_init();
#endif

    // initialise data structures.
    passgen_cli_opts_init(&opts);
    if(!opts.env) {
        passgen_cli_opts_free(&opts);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    // load values from a config file (located at /etc/passgen.conf, or
    // $HOME/.config/passgen.conf).
    ret = passgen_cli_opts_config(&opts);
    if(ret != PASSGEN_CLI_ERROR_NONE) {
        passgen_cli_opts_free(&opts);
        return ret;
    }

    // parse CLI options, which override the previously loaded configuration.
    ret = passgen_cli_opts_parse(&opts, argc, argv);
    if(ret == PASSGEN_CLI_SHOW_HELP) {
        passgen_cli_usage(argv[0]);
        passgen_cli_opts_free(&opts);
        return 0;
    } else if(ret == PASSGEN_CLI_SHOW_VERSION) {
        passgen_cli_show_version();
        passgen_cli_opts_free(&opts);
        return 0;
    } else if(ret != PASSGEN_CLI_ERROR_NONE) {
        passgen_cli_opts_free(&opts);
        return ret;
    }

    // run the cli commands and free memory.
    ret = passgen_cli_run(opts);
    passgen_cli_opts_free(&opts);

    return ret;
}
