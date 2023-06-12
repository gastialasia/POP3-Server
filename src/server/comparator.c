//Parser comparator: su funcion es matchear el comando en el array procesado por tokenizer con la lista de comandos permitidos para el estado actual de forma que el flujo de la ejecucion pueda derivarse en el handler correspondiente
#include "../include/pop3nio.h"
#include "../include/parser.h"
#include "../include/comparator.h"
#include <string.h>

#define AUTH_COMMAND_QTY 4
#define COMMAND_LEN 5

#define CAPA_MSG "+OK\nCAPA\nUSER\nPIPELINING\n.\r\n"
#define INVALID_COMMAND_MSG "-ERR Unknown command.\r\n"

typedef struct{
    char command_id[COMMAND_LEN];
    unsigned int(*command_handler)(buffer*);
} command_type;

unsigned int noop(){
    printf("Func blanca\n");
    return 0;
}

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

unsigned int capa_handler(buffer *b) {
    return write_to_buffer(CAPA_MSG, b);
}

unsigned int invalid_command_handler(buffer *b) {
    return write_to_buffer(INVALID_COMMAND_MSG, b);
}


static command_type auth_commands[AUTH_COMMAND_QTY] = {
    {.command_id = "CAPA", .command_handler = &capa_handler},
    {.command_id = "USER", .command_handler = &noop},
    {.command_id = "PASS", .command_handler = &noop},
    {.command_id = "QUIT", .command_handler = &noop},
};

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


