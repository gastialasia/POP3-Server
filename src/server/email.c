//email.c: Email managing functions, such as navigating between paths

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/email.h"
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
    while(d != NULL && strcmp(d->d_name, "mail1")){
        d = readdir(directory);
    }
        

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

void load_mails(struct pop3 * p3) {
    DIR * directory = open_maildir(p3, INITIAL_PATH);
    struct dirent * d = readdir(directory);
    struct stat file_statistics;
    while(d != NULL){
        if (strcmp(d->d_name,".") && strcmp(d->d_name,"..")){
            struct mail_t * new = malloc(sizeof(struct mail_t)); //Creamos mail
            new->filename = NULL; //Por ahora no lo necesitamos
            new->marked_del = 0;
            
            //MODULARIZAR
            size_t user_len = strlen(p3->credentials->user);
            size_t path_len = strlen(INITIAL_PATH); 
            size_t file_name_len = strlen(d->d_name);
            char* mail_path = malloc((user_len+path_len+file_name_len+strlen(CUR)+1)*sizeof(char)); //+1 por el null terminated
            strncpy(mail_path, INITIAL_PATH, path_len+1);
            strncat(mail_path, p3->credentials->user, user_len);
            strcat(mail_path, CUR);
            strcat(mail_path, d->d_name);

            if(stat(mail_path, &file_statistics)){ //Si sale mal, stat devuelve algo distinto de 0 y entonces entro
                //Manejar el error
            }

            //cargamos el mail en cuestion
            printf("Cargando mail (%s): %s\n",p3->credentials->user,d->d_name);
            new->size = file_statistics.st_size; //Aca hay que hacer stat
            p3->mails[p3->mail_qty++] = new; //Guardo mail en el array de mails
        
            //añadimos la cantidad de bytes del mail al total del inbox
            
            p3->total_octates += file_statistics.st_size;
        }
        d = readdir(directory);
    }
    p3->max_index = p3->mail_qty-1;
    p3->original_total_octates = p3->total_octates;
}