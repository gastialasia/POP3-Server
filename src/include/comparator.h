#ifndef COMPARATOR_H
#define COMPARATOR_H

#include <stdio.h>
#include "../include/buffer.h"

typedef unsigned int(*fn_type)(buffer*);

unsigned int capa_handler(buffer *b);

fn_type comparator(struct parser_event * pe, unsigned int curr_state);

#endif 