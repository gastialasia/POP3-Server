//email.c: Email managing functions, such as navigating between paths


#include "../include/email.h"

#define CUR "/cur/"

DIR* open_maildir(struct pop3* p3, char* path){
    size_t user_len = strlen(p3->credentials->user);
    size_t path_len = strlen(path); 
    char* full_path = malloc((user_len+path_len+strlen(CUR)+1)*sizeof(char)); //+1 por el null terminated
    strncpy(full_path, path, path_len+1);
    strncat(full_path, p3->credentials->user, user_len);
    strcat(full_path, CUR);
    DIR* ret = opendir(full_path);
    free(full_path);
    return ret; //En el handler tenemos que chequear si es null para pasar la stm a ERROR
}

char* read_mail(DIR* directory, struct pop3* p3, char* path){
    if(directory == NULL)
        printf("Si, era NULL\n");
    struct dirent* d = readdir(directory);
    while(d != NULL && strcmp(d->d_name, "mail1"))
        d = readdir(directory);

    //Creamos el path al archivo de mail
    size_t user_len = strlen(p3->credentials->user);
    size_t path_len = strlen(path); 
    size_t file_name_len = strlen(d->d_name);
    char* mail_path = malloc((user_len+path_len+file_name_len+strlen(CUR)+1)*sizeof(char)); //+1 por el null terminated
    strncpy(mail_path, path, path_len+1);
    strncat(mail_path, p3->credentials->user, user_len);
    strcat(mail_path, CUR);
    strcat(mail_path, d->d_name);
    printf("%s\n", mail_path);

    FILE* mail = fopen(mail_path, "r");
    char* buf = malloc(100*sizeof(char));
    fgets(buf, 100, mail);
    fclose(mail);
    closedir(directory);
    return buf;
}