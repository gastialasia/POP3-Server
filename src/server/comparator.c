//Parser comparator: su funcion es matchear el comando en el array procesado por tokenizer con la lista de comandos permitidos para el estado actual de forma que el flujo de la ejecucion pueda derivarse en el handler correspondiente
#include "../include/parser.h"
#include "../include/comparator.h"
#include "../include/pop3nio.h"
#include "../include/email.h"

#include <string.h>
#include <stdlib.h>


#define AUTH_COMMAND_QTY 4
#define TR_COMMAND_QTY 8

#define STATE_QTY 2

#define COMMAND_LEN 5

#define CAPA_MSG "+OK\nCAPA\nUSER\nPIPELINING\n.\r\n"
#define INVALID_COMMAND_MSG "-ERR Unknown command.\r\n"
#define PASS_NOUSER_MSG "-ERR No username given.\r\n"
#define AUTH_ERROR_MSG "-ERR [AUTH] Authentication failed.\r\n"
#define INVALID_INDEX_MSG "-ERR Invalid message number\n"
#define NO_MSG_MSG "-ERR No such message\n"
#define LIST_FMT "+OK %u messages (%ld octets)\n"
#define LOGIN_MSG "+OK Logged in.\r\n"
#define STAT_FMT "+OK %u %ld\n"
#define POSITIVE_MSG "+OK\n"
#define NEGATIVE_MSG "-ERR\n"

#define TEST_USER "foo"
#define TEST_PASS "bar"

typedef struct{
    char command_id[COMMAND_LEN];
    fn_type command_handler;
} command_type;


static command_type auth_commands[AUTH_COMMAND_QTY] = {
    {.command_id = "CAPA", .command_handler = &capa_handler},
    {.command_id = "USER", .command_handler = &user_handler},
    {.command_id = "PASS", .command_handler = &pass_handler},
    {.command_id = "QUIT", .command_handler = &noop},
};

static command_type transaction_commands[TR_COMMAND_QTY] = {
    {.command_id = "NOOP", .command_handler = &noop},
    {.command_id = "STAT", .command_handler = &stat_handler},
    {.command_id = "LIST", .command_handler = &list_handler},
    {.command_id = "RETR", .command_handler = &noop},
    {.command_id = "DELE", .command_handler = &noop},
    {.command_id = "RSET", .command_handler = &rset_handler},
    {.command_id = "CAPA", .command_handler = &noop},
    {.command_id = "QUIT", .command_handler = &noop},
};

static command_type* commands_per_state[STATE_QTY] = {auth_commands, transaction_commands};
static int qty_per_state[STATE_QTY] = {AUTH_COMMAND_QTY, TR_COMMAND_QTY};

unsigned int write_to_buffer(char * str, buffer *b){
    size_t count;
    size_t len = strlen(str);
    uint8_t* buf = buffer_write_ptr(b, &count);
    if(count < len){
        return 2;
    }
    memcpy(buf, str, len);
    buffer_write_adv(b, len);
    return 0;
}

//Devuelve 1 si cumplen las credenciales, 0 si hay algo mal
int validate_credentials(struct pop3 * p3, char * pass){
    return !strcmp(p3->credentials->user, TEST_USER) && !strcmp(pass, TEST_PASS);
}

/*unsigned int noop(buffer*b, struct pop3*p3, char *arg1, char* arg2){
    write_to_buffer(POSITIVE_MSG, b);
    return p3->stm.current->state;
}*/

unsigned int noop(buffer*b, struct pop3*p3, char *arg1, char* arg2){
    //printf("%s\n", read_mail(open_maildir(p3, INITIAL_PATH), p3, INITIAL_PATH));
    load_mails(p3);
    return p3->stm.current->state;
}

unsigned int rset_handler(buffer*b, struct pop3*p3, char *arg1, char* arg2){
    struct mail_t** aux = p3->mails;
    for(unsigned int i=0; i < p3->mail_qty; i++){
        aux[i]->marked_del = 0;
    }
    return p3->stm.current->state;
}

unsigned int list_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    //Falta if para elegir mensaje segun el estado
    printf("el arg1 %s NULL\n", arg1 == NULL?"es":"no es");
    char list_msg[40];
    if(arg1 != NULL){
        unsigned int index = atoi(arg1);
        if (index==0){
            write_to_buffer(INVALID_INDEX_MSG, b); //Crear define
            return TRANSACTION;
        }
        if (index > p3->mail_qty){
            write_to_buffer(NO_MSG_MSG, b); //Crear define
            return TRANSACTION;
        }
        sprintf(list_msg, STAT_FMT, index, p3->mails[index-1]->size);  //Si me pasan 1, quiero mails[0]
        write_to_buffer(list_msg, b);
    } else {
        //LIST sin argumentos
        sprintf(list_msg, LIST_FMT, p3->mail_qty, p3->total_octates); 
        write_to_buffer(list_msg, b);
        unsigned int i;
        for(i=0; i < p3->mail_qty; i++){
            sprintf(list_msg, "%u %ld\n",i+1,p3->mails[i]->size);
            write_to_buffer(list_msg, b); 
        }
        write_to_buffer(".\n", b);
    }
    return TRANSACTION;
}

unsigned int stat_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    //Falta if para elegir mensaje segun el estado
    char stat_msg[40];
    sprintf(stat_msg, STAT_FMT, p3->mail_qty, p3->total_octates); 
    write_to_buffer(stat_msg, b);
    return TRANSACTION;
}

unsigned int capa_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    //Falta if para elegir mensaje segun el estado
    write_to_buffer(CAPA_MSG, b);
    return AUTH;
}

unsigned int invalid_command_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    write_to_buffer(INVALID_COMMAND_MSG, b);
    //Falta if
    return p3->stm.current->state; 
}

unsigned int user_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    if (arg1!=NULL){
        int len = strlen(arg1);
        p3->credentials->user = malloc(len*sizeof(char)+1);
        strncpy(p3->credentials->user, arg1, len+1);
    }
    write_to_buffer(POSITIVE_MSG, b);
    return AUTH;
}

unsigned int pass_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    if (arg1!=NULL){
        if(p3->credentials->user==NULL){
            return write_to_buffer(PASS_NOUSER_MSG, b);
        }
        if (validate_credentials(p3, arg1)){
            int len = strlen(arg1);
            p3->credentials->pass = malloc(len*sizeof(char)+1);
            strncpy(p3->credentials->pass, arg1, len+1);
            //load_user_data(pop3);
            write_to_buffer(LOGIN_MSG, b);
            return TRANSACTION;
        } 
        
    }
    write_to_buffer(AUTH_ERROR_MSG, b);
    return AUTH;
}

fn_type comparator(struct parser_event * pe, unsigned int curr_state){
    printf("el current state es: %u\n", curr_state);
    char * command = pe->commands[0];
    command_type* command_list = commands_per_state[curr_state];
    int command_qty = qty_per_state[curr_state];
    command_type curr_command;
    //void * ret = &blank_function; // Funcion de error por default
    for(int i=0; i<command_qty; i++){
        curr_command = command_list[i];
        if (!strcmp(curr_command.command_id, command)){
            return curr_command.command_handler;
        }
    }
    return &invalid_command_handler;
}


