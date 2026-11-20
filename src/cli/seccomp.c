/// @file seccomp.c
///
/// Restricts which system calls the command-line tool may make, with seccomp.
///
/// This is not part of the unity build: it includes <asm/termbits.h>, which
/// declares its own struct termios, and so cannot be in the same translation
/// unit as <termios.h> (which prompt.c includes).
#include "passgen.h"
#include <fcntl.h>
#include <seccomp.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
// TCGETS2 is defined with the size of struct termios2, which only the kernel
// headers declare. this is fine as long as <termios.h> is not included, which
// declares its own struct termios.
#ifdef __linux__
#include <asm/termbits.h>
#endif

// musl uses this, glibc doesn't have it by default.
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

void passgen_cli_seccomp_init(void) {
    scmp_filter_ctx ctx;
    ctx = seccomp_init(SCMP_ACT_KILL);

    // allow open syscalls, as long as they are read-only.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(open),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, O_RDONLY));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(openat),
        1,
        SCMP_CMP(2, SCMP_CMP_EQ, O_RDONLY));
    // musl sets O_LARGEFILE. architectures which only have openat (such as
    // aarch64 and riscv64) need it there as well.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(open),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, O_RDONLY | O_LARGEFILE));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(openat),
        1,
        SCMP_CMP(2, SCMP_CMP_EQ, O_RDONLY | O_LARGEFILE));

    // allow reading stat of files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);

    // allow reading and seeking in files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(readv), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);

    // allow writing, but only to stdout or stderr.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 1));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 2));
    // musl uses writev
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(writev),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 1));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(writev),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 2));

    // allow closing, as long as the fd is not any of the standard streams.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(close),
        1,
        SCMP_CMP(0, SCMP_CMP_GT, 2));

    // allow reading random data
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);

    // newer versions of glibc (2.41 and later) implement getrandom() in the
    // vDSO, and the first call blocks signals while it sets up its state.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);

    // allow exiting
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);

    // allow allocating memory
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);

    // allow mapping anonymous memory (used by allocator)
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(mmap),
        3,
        SCMP_CMP(0, SCMP_CMP_EQ, 0),
        SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_WRITE),
        SCMP_CMP(3, SCMP_CMP_EQ, MAP_PRIVATE | MAP_ANONYMOUS));

    // the state of getrandom() in the vDSO is in memory which the kernel may
    // drop (MAP_DROPPABLE, Linux 6.11 and later). headers which are older
    // than that do not define it, but glibc may still use it.
#ifndef MAP_DROPPABLE
#define MAP_DROPPABLE 0x08
#endif
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(mmap),
        3,
        SCMP_CMP(0, SCMP_CMP_EQ, 0),
        SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_WRITE),
        SCMP_CMP(3, SCMP_CMP_EQ, MAP_DROPPABLE | MAP_ANONYMOUS));

    // allow unmapping memory (used by allocation)
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);

    // allow remapping memory (used by allocation)
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mremap), 0);

    // allow advice about memory, which is only about the memory of this
    // process. the allocator of glibc uses it for large allocations (such as
    // the memory for deriving the key from a master passphrase, if
    // transparent huge pages are enabled), and when freeing memory.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(madvise), 0);

    // allow checking for terminal window size
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        2,
        SCMP_CMP(0, SCMP_CMP_EQ, 1),
        SCMP_CMP(1, SCMP_CMP_EQ, TIOCGWINSZ));

    // allow checking whether a file is a terminal (which only reads its
    // settings). the C library does this when it starts reading from or
    // writing to a character device, such as /dev/null or /dev/urandom, to
    // decide how to buffer: without this, `passgen a > /dev/null` and
    // `passgen -r file:/dev/urandom a` were killed. newer versions of glibc
    // use TCGETS2 on some architectures.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, TCGETS));
#ifdef TCGETS2
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, TCGETS2));
#endif

    // allow turning off showing what is typed in the terminal (and back on),
    // to read a master passphrase from standard input. newer versions of
    // glibc use TCSETS2 and friends on some architectures.
    const unsigned long tcsets[] = {
        TCSETS,
        TCSETSW,
        TCSETSF,
#ifdef TCSETS2
        TCSETS2,
        TCSETSW2,
        TCSETSF2,
#endif
    };
    for(size_t i = 0; i < sizeof(tcsets) / sizeof(tcsets[0]); i++) {
        seccomp_rule_add(
            ctx,
            SCMP_ACT_ALLOW,
            SCMP_SYS(ioctl),
            2,
            SCMP_CMP(0, SCMP_CMP_EQ, 0),
            SCMP_CMP(1, SCMP_CMP_EQ, tcsets[i]));
    }

    // allow setting what happens on being interrupted, which sets the
    // terminal back while reading a master passphrase.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);

    // apply rules
    seccomp_load(ctx);
    seccomp_release(ctx);
}
