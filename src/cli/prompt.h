/// @file prompt.h
///
/// Reading secrets, such as a master passphrase, from the terminal.
#pragma once
#include <stdbool.h>
#include <stddef.h>

/// What reading a secret can result in.
typedef enum {
    /// A line was read.
    PASSGEN_CLI_PROMPT_OK,
    /// There was nothing to read, or it could not be read.
    PASSGEN_CLI_PROMPT_EOF,
    /// The line is too long for the buffer.
    PASSGEN_CLI_PROMPT_TOO_LONG,
} passgen_cli_prompt_status;

/// Whether secrets are read from a terminal, so that it makes sense to ask
/// for one twice, to make sure it was typed correctly.
bool passgen_cli_prompt_interactive(void);

/// Reads a secret, such as a master passphrase. When standard input is a
/// terminal, the prompt is shown on standard error, and what is typed is not
/// shown. Otherwise, a line is read from standard input, without a prompt.
///
/// The line is read without the newline at its end. It is read one byte at
/// a time, so that it is not kept in a buffer anywhere else. The caller has
/// to wipe it once it is not needed anymore.
///
/// @param prompt What to show before reading
/// @param buffer Where to store it, as a NUL-terminated string
/// @param size Size of the buffer, including the NUL terminator
passgen_cli_prompt_status
passgen_cli_prompt_secret(const char *prompt, char *buffer, size_t size);
