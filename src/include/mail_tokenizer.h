#ifndef MAIL_TOKENIZER_H
#define MAIL_TOKENIZER_H

#include "parser.h"

enum mail_states
{
    NEW_LINE,
    DOT,
    DOT_CR,
    BYTE,
    CR,
    FIN
};

struct parser *create_mail_parser();

#define MAIL_STATEQTY 6

#endif