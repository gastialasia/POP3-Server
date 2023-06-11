//Parser comparator: su funcion es matchear el comando en el array procesado por tokenizer con la lista de comandos permitidos para el estado actual de forma que el flujo de la ejecucion pueda derivarse en el handler correspondiente
#include "../include/pop3nio.h"

#define AUTH_COMMAND_QTY 4
#define COMMAND_LEN 5

typedef struct{
    char command_id[COMMAND_LEN];
    void* command_handler;
} command_type;

static command_type auth_commands[AUTH_COMMAND_QTY] = {
    {.command_id = 'CAPA', .command_handler = &handler_capa},
    {.command_id = 'USER', .command_handler = &handler_user},
    {.command_id = 'PASS', .command_handler = &handler_pass},
    {.command_id = 'QUIT', .command_handler = &handler_quit},
};

bool match_command(char** commands, pop3state){

}