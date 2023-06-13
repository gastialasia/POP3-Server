//email.c: Email managing functions, such as navigating between paths


#include "../include/email.h"

#define CUR "/cur"

DIR* open_maildir(struct pop3* p3, char* path){
    size_t user_len = strlen(p3->credentials->user);
    size_t path_len = strlen(path); 
    char* full_path = malloc((user_len+path_len+4+1)*sizeof(char)); //+1 por el null terminated
    strncpy(full_path, path, path_len+1);
    strncat(full_path, p3->credentials->user, user_len);
    strcat(full_path, "/cur");
    DIR* ret = opendir(full_path);
    return ret; //En el handler tenemos que chequear si es null para pasar la stm a ERROR
}

char* read_mail(DIR* directory){
    struct dirent* d = readdir(directory);
    printf("el nombre es: %s\n", d->d_name);
    FILE* mail = fopen("./directories/foo/cur/", "r");
    char* buf = malloc(100*sizeof(char));
    printf("%s\n", buf);
    fgets(buf, 100, mail);
    fclose(mail);
    closedir(directory);
    return buf;
}