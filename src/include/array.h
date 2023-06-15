#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

#include "email.h"

typedef struct mail_arrayCDT * mail_array;

struct mail_arrayCDT
{
    struct mail_t ** mails;
    unsigned qty;
};

//mails en null y qty en 0
mail_array init_array();
int add_mail(mail_array array, struct mail_t* mail);
struct mail_t* get_mail(mail_array array, unsigned index);

#endif