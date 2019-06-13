#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include "random.h"

void usage(const char *executable);
void error(const char *err);

int main(int argc, char *argv[])
{
    // presets
    const char *alpha_small = "abcdefghijklmnopqrstuvwxyz";
    const char *alpha_caps = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *numeric = "0123456789";
    const char *symbols = "!@#$%^&*([{}])=+|<>?";

    // command line flags
    int opt_amount = 10;
    int opt_length = 8;
    bool opt_manual = 0;
    bool opt_alpha_small = 0;
    bool opt_alpha_caps = 0;
    bool opt_numeric = 0;
    bool opt_symbols = 0;
    char *opt_custom = NULL;

    // parses the command line flags
    for(int i = 1; i < argc; i++) {
        if((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            usage(argv[0]);
            return 0;
        } else if((strcmp(argv[i], "-a") == 0) || (strcmp(argv[i], "--amount") == 0)) {
            if((i+1) < argc) {
                opt_amount = atoi(argv[i+1]);
                i++;
            } else {
                usage(argv[0]);
                return -1;
            }
        } else if((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "--length") == 0)) {
            if((i+1) < argc) {
                opt_length = atoi(argv[i+1]);
                i++;
            } else {
                usage(argv[0]);
                return -1;
            }
        } else if((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--custom") == 0)) {
            if((i+1) < argc) {
                opt_custom = argv[i+1];
                opt_manual = 1;
                i++;
            } else {
                usage(argv[0]);
                return -1;
            }
        } else if((strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "--numeric") == 0)) {
            opt_manual = opt_numeric = 1;
        } else if((strcmp(argv[i], "-as") == 0) || (strcmp(argv[i], "--alpha-small") == 0)) {
            opt_manual = opt_alpha_small = 1;
        } else if((strcmp(argv[i], "-ac") == 0) || (strcmp(argv[i], "--alpha-caps") == 0)) {
            opt_manual = opt_alpha_caps = 1;
        } else if(strcmp(argv[i], "--alpha") == 0) {
            opt_manual = opt_alpha_small = opt_alpha_caps = 1;
        } else if((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--symbols") == 0)) {
            opt_manual = opt_symbols = 1;
        } else {
            usage(argv[0]);
            return -1;
        }
    }

    // turn on alphanumeric if the user didn't specify anything
    if(!opt_manual) {
        opt_alpha_small = opt_alpha_caps = opt_numeric = 1;
    }

    // if the amonut is set to something illegal, rectify it
    if(opt_amount <= 0) {
        opt_amount = 10;
    }

    // if the length is set to something illegal, rectify it
    if(opt_length <= 0) {
        opt_length = 8;
    }


    // construct the string of possible chars
    int charlength = 0;
    charlength += (opt_alpha_small) ? strlen(alpha_small) : 0;
    charlength += (opt_alpha_caps) ? strlen(alpha_caps) : 0;
    charlength += (opt_numeric) ? strlen(numeric) : 0;
    charlength += (opt_symbols) ? strlen(symbols) : 0;
    charlength += (opt_custom) ? strlen(opt_custom) : 0;

    char *charlist = malloc(charlength+1);
    if(charlist == NULL) {
        error("can't allocate memory!");
    }
    memset(charlist, 0, (charlength+1));

    if(opt_custom) {
        strcpy(charlist, opt_custom);
    }

    if(opt_alpha_small) {
        strcpy((charlist + strlen(charlist)), alpha_small);
    }

    if(opt_alpha_caps) {
        strcpy((charlist + strlen(charlist)), alpha_caps);
    }

    if(opt_numeric) {
        strcpy((charlist + strlen(charlist)), numeric);
    }

    if(opt_symbols) {
        strcpy((charlist + strlen(charlist)), symbols);
    }

    // initialize source of random numbers
    random_t *random = random_new();
    assert(random != NULL);

    // generate passwords
    for(int i = 0; i < opt_amount; i++) {
        for(int j = 0; j < opt_length; j++) {
            // get random number
            //unsigned int rnd = havege_rand(&hs);
            unsigned int rnd = random_uint64_max(random, charlength);

            // limit it from 0 to charlength
            rnd = rnd;

            // print random character
            printf("%c", charlist[rnd]);
        }

        printf("\t");
    }

    random_close(random);
    free(charlist);

    printf("\n");
    return 0;
}

void usage(const char *executable) {
    printf("Usage: %s [-h|-a|-l|-c|-s|-as|-ac] ...\n", executable);
    printf("  -h|--help         Show this usage information\n");
    printf("  -a|--amount       The amount of passwords\n");
    printf("  -l|--length       The length of the passwords\n");
    printf("  -c|--custom       Include custom characters\n");
    printf("  -s|--symbols      Include symbols in the passwords\n");
    printf("  -n|--numeric      Include numeric characters\n");
    printf("  --alpha           Include alphabetical characters\n");
    printf("  -as|--alpha-small Include small alphabetical characters\n");
    printf("  -ac|--alpha-caps  Include capitalized alphabetical characters\n");
}

void error(const char *err) {
    printf("%s", err);
    exit(-1);
}
