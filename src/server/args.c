#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>
#include "../include/args.h"

#define INITIAL_PATH "./directories/"

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        "   -m <maildir>  Dirección del maildir\n"
        "   -p <port>  Puerto entrante conexiones.\n"
        "\n",
        progname);
    exit(1);
}

void 
parse_args(const int argc, char **argv, struct pop3args *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users


    args->mng_addr   = "127.0.0.1";
    args->port   = 1100;
    strncpy(args->path, INITIAL_PATH, strlen(INITIAL_PATH)+1);

    int c;

    while (true) {

        c = getopt(argc, argv, "hp:m:");
        if (c == -1)
            break;
        int len;
        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'p':
                args->port = port(optarg);
                break;
            case 'm':
                len = strlen(optarg);
                if(len>254){
                    len=254;
                }
                strncpy(args->path, optarg, len+1);
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
