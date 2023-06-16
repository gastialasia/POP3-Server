#ifndef MAIL_TOKENIZER_H
#define MAIL_TOKENIZER_H

#include "parser.h"

enum states
{
    NEW_LINE,
    DOT,
    DOT_CR,
    BYTE,
    CR,
    FIN
};

#define STATEQTY 6

#endif