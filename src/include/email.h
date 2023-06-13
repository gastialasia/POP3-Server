#ifndef EMAIL_H
#define EMAIL_H
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/pop3nio.h"

DIR* open_maildir(struct pop3* p3, char* path);

char* read_mail(DIR* directory);

#endif