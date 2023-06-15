#include "../include/array.h"

#define BLOCK 5

mail_array init_array(){
    return calloc(1, sizeof(struct mail_arrayCDT)); 
}

int add_mail(mail_array array, struct mail_t* mail){
    if(array->qty%BLOCK == 0){
        array->mails = realloc(array->mails, (array->qty+BLOCK)*sizeof(struct mail_t));
        if(array->mails == NULL){
            return 1;
        }
    }
    array->mails[array->qty++] = mail;
    return 0;
}

struct mail_t* get_mail(mail_array array, unsigned index){
    if(index >= array->qty){
        return NULL;
    }
    return array->mails[index];
}

