#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "buffer.h"
#include "tokenizer.h"

enum auth_state {
    auth_no_user, //todavia no se identifico el usuario
    auth_no_pass, //todavia no se ingreso la contrasena
    auth_done,
    auth_error_invalid,
};

struct auth_parser {
    /** permite al usuario del parser almacenar sus datos */
    void *data;
    struct parser * parser;
    /******** zona privada *****************/
    enum auth_state state;
};

#endif