#ifndef EMAIL_H
#define EMAIL_H
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/pop3nio.h"

// Estructura para procesar mails al entrar al pasar a TRANSACTION
struct mail_t
{
    char *file_path;
    size_t size;
};

DIR *open_maildir(struct pop3 *p3, char *path);

char *read_mail(DIR *directory, struct pop3 *p3, char *path);

// Carga los mails en la estructura pop3
void load_mails(struct pop3 *p3);

void free_mails(struct pop3 *p3);

int change_maildir(char *new_path);

#endif
