//Parser comparator: su funcion es matchear el comando en el array procesado por tokenizer con la lista de comandos permitidos para el estado actual de forma que el flujo de la ejecucion pueda derivarse en el handler correspondiente
#include "../include/pop3nio.h"
#include "../include/parser.h"
#include "../include/comparator.h"
#include <string.h>

#define AUTH_COMMAND_QTY 4
#define COMMAND_LEN 5

typedef struct{
    char command_id[COMMAND_LEN];
    void* command_handler;
} command_type;

static void * blank_function(){
    printf("Func blanca\n");
}

static void * capa_handler(){
    printf("Estoy haciendo CAPA!!!!\n");
}

static command_type auth_commands[AUTH_COMMAND_QTY] = {
    {.command_id = 'CAPA', .command_handler = &capa_handler},
    {.command_id = 'USER', .command_handler = &blank_function},
    {.command_id = 'PASS', .command_handler = &blank_function},
    {.command_id = 'QUIT', .command_handler = &blank_function},
};

void * comparator(struct parser_event * pe, unsigned int curr_state){
    char * command = pe->commands[0];
    command_type curr_command;
    //void * ret = &blank_function; // Funcion de error por default
    for(int i=0; i<AUTH_COMMAND_QTY; i++){
        curr_command = auth_commands[i];
        if (!strcmp(curr_command.command_id, command)){
            return curr_command.command_handler;
        }
    }
    return &blank_function;
}