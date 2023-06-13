//Parser comparator: su funcion es matchear el comando en el array procesado por tokenizer con la lista de comandos permitidos para el estado actual de forma que el flujo de la ejecucion pueda derivarse en el handler correspondiente
#include "../include/parser.h"
#include "../include/comparator.h"
#include <string.h>
#include <stdlib.h>

#define AUTH_COMMAND_QTY 4
#define COMMAND_LEN 5

#define CAPA_MSG "+OK\nCAPA\nUSER\nPIPELINING\n.\r\n"
#define INVALID_COMMAND_MSG "-ERR Unknown command.\r\n"
#define PASS_NOUSER_MSG "-ERR No username given.\r\n"
#define AUTH_ERROR_MSG "-ERR [AUTH] Authentication failed.\r\n"
#define LOGIN_MSG "+OK Logged in.\r\n"
#define POSITIVE_MSG "+OK\n"
#define NEGATIVE_MSG "-ERR\n"

#define TEST_USER "foo"
#define TEST_PASS "bar"

typedef struct{
    char command_id[COMMAND_LEN];
    fn_type command_handler;
} command_type;

unsigned int noop(){
    printf("Func blanca\n");
    return 0;
}

static command_type auth_commands[AUTH_COMMAND_QTY] = {
    {.command_id = "CAPA", .command_handler = &capa_handler},
    {.command_id = "USER", .command_handler = &user_handler},
    {.command_id = "PASS", .command_handler = &pass_handler},
    {.command_id = "QUIT", .command_handler = &noop},
};

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

unsigned int capa_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    //Falta if para elegir mensaje segun el estado
    write_to_buffer(CAPA_MSG, b);
    return AUTH;
}

unsigned int invalid_command_handler(buffer *b, struct pop3 *p3, char *arg1, char *arg2) {
    write_to_buffer(INVALID_COMMAND_MSG, b);
    //Falta if
    return AUTH; 
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
    char * command = pe->commands[0];
    //command_type curr_command;
    //void * ret = &blank_function; // Funcion de error por default
    for(int i=0; i<AUTH_COMMAND_QTY; i++){
        //curr_command = auth_commands[i];
        if (!strcmp(auth_commands[i].command_id, command)){
            return auth_commands[i].command_handler;
        }
    }
    return &invalid_command_handler;
}


