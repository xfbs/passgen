#include "prompt.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// what the terminal was set to before echoing was turned off, which is set
// back when being interrupted.
static struct termios prompt_saved;

static void prompt_restore(int signal) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &prompt_saved);
    // the newline which was not echoed.
    (void) !write(STDERR_FILENO, "\n", 1);
    _exit(128 + signal);
}

bool passgen_cli_prompt_interactive(void) {
    return isatty(STDIN_FILENO);
}

passgen_cli_prompt_status
passgen_cli_prompt_secret(const char *prompt, char *buffer, size_t size) {
    bool terminal = passgen_cli_prompt_interactive();
    struct sigaction previous_int, previous_term;

    if(terminal) {
        fprintf(stderr, "%s", prompt);
        fflush(stderr);

        // what is typed is not shown, but the newline at its end is.
        if(0 == tcgetattr(STDIN_FILENO, &prompt_saved)) {
            struct termios hidden = prompt_saved;
            hidden.c_lflag &= ~(tcflag_t) ECHO;
            hidden.c_lflag |= ECHONL;

            // being interrupted would leave the terminal like this.
            struct sigaction action;
            memset(&action, 0, sizeof(action));
            action.sa_handler = prompt_restore;
            sigemptyset(&action.sa_mask);
            sigaction(SIGINT, &action, &previous_int);
            sigaction(SIGTERM, &action, &previous_term);

            tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden);
        } else {
            terminal = false;
        }
    }

    size_t length = 0;
    bool too_long = false;
    bool any = false;
    while(true) {
        char c;
        ssize_t count = read(STDIN_FILENO, &c, 1);
        if(count != 1 || c == '\n') {
            any = any || count == 1;
            break;
        }
        any = true;
        if(length + 1 < size) {
            buffer[length++] = c;
        } else {
            too_long = true;
        }
        c = 0;
    }

    // a line which ends in CRLF.
    if(length && buffer[length - 1] == '\r') {
        length--;
    }
    buffer[length] = 0;

    if(terminal) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &prompt_saved);
        sigaction(SIGINT, &previous_int, NULL);
        sigaction(SIGTERM, &previous_term, NULL);
    }

    if(!any) {
        return PASSGEN_CLI_PROMPT_EOF;
    }
    if(too_long) {
        memset(buffer, 0, size);
        return PASSGEN_CLI_PROMPT_TOO_LONG;
    }
    return PASSGEN_CLI_PROMPT_OK;
}
