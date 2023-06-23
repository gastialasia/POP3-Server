//email.c: Email managing functions, such as navigating between paths

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/email.h"
#include "../include/email.h"

#define CUR "/cur/"
#define INITIAL_PATH "./directories/"

#define BLOCK 20

char* path_to_maildir = INITIAL_PATH;

int change_maildir(char * new_path){
  if(strcmp(path_to_maildir,INITIAL_PATH) != 0){
    free(path_to_maildir);//hago un free de un path viejo
  }
  int len = strlen(new_path) + 1;
  char * path = malloc(len);
  if(path == NULL){
    return -1;
  }
  strcpy(path,new_path);
  path_to_maildir = path;
  return 0;
}



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
        printf("Error opening inbox directory.\n");
    struct dirent* d = readdir(directory);
    while(d != NULL){
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

    FILE* mail = fopen(mail_path, "r");
    char* buf = malloc(100*sizeof(char));
    fgets(buf, 100, mail);
    fclose(mail);
    closedir(directory);
    return buf;
}

void load_mails(struct pop3 * p3) {
    DIR * directory = open_maildir(p3, path_to_maildir);
    struct dirent * d = readdir(directory);
    struct stat file_statistics;
    if(p3->max_index != 0){ //Chequeo si ya habia mails cargados
        free_mails(p3);
    }
    p3->mail_qty = 0;
    p3->max_index = 0;
    p3->total_octates = 0;
    unsigned int i=0;
    while(d != NULL){
        if (strcmp(d->d_name,".") && strcmp(d->d_name,"..")){

            struct mail_t * new = malloc(sizeof(struct mail_t)); //Creamos mail
            
            //MODULARIZAR
            size_t user_len = strlen(p3->credentials->user);
            size_t path_len = strlen(path_to_maildir); 
            size_t file_name_len = strlen(d->d_name);
            new->file_path = malloc((user_len+path_len+file_name_len+strlen(CUR)+1)*sizeof(char)); //+1 por el null terminated
            strncpy(new->file_path, path_to_maildir, path_len+1);
            strncat(new->file_path, p3->credentials->user, user_len);
            strcat(new->file_path, CUR);
            strcat(new->file_path, d->d_name);

            if(stat(new->file_path, &file_statistics)){ //Si sale mal, stat devuelve algo distinto de 0 y entonces entro
                //Manejar el error
            }

            new->size = file_statistics.st_size; 

            //p3->mails = malloc(BLOCK*sizeof(struct mail_t*)); 

            //Cargo mail en el array. Si hace falta, lo agrando
            if (i%BLOCK==0){
                p3->mails = realloc(p3->mails, (BLOCK+i)*sizeof(struct mail_t*)); //realloco espacio para mails
                p3->dele_flags = realloc(p3->dele_flags, (BLOCK+i)*sizeof(u_int8_t)); //realloco espacio para flags
                if (p3->mails==NULL||p3->dele_flags==NULL){
                    printf("ERROR: Not enough memory to load user mails\n");
                }
                for(unsigned int j=i; j<i+BLOCK; j++){
                    p3->dele_flags[j]=0; //Seteo los flags recien creados en 0
                }
            }
            
            p3->mails[i] = new; //Guardo mail en el array de mails

            if(!p3->dele_flags[i]){
                //Si el mail no esta marcado como borrado:
                p3->mail_qty++; //Lo agrego a mail_qty, entonces esta actualizado cada vez que cargo los mails
                p3->total_octates += file_statistics.st_size; //Sumo sus octetos
            }

            i++;
        }
        d = readdir(directory);
    }
    p3->max_index = i-1; //Le resto uno porque despues del while me quedo incrementado en 1
    p3->original_total_octates = p3->total_octates;
    closedir(directory);
    for(unsigned int i=0; i<=p3->max_index; i++){
    }
}

void free_mails(struct pop3 * p3) {
    if (p3->mails==NULL){
        return;
    }  
    for(unsigned i=0; i<=p3->max_index; i++){
        free(p3->mails[i]->file_path);
        free(p3->mails[i]);
    }
}
