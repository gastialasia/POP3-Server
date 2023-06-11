//Parser tokenizer: es responsable de consumir la entrada y separarla en un array [comando, argumento1, argumento2]

#include <stdio.h>
#include <../include/parser.h>
#include "../include/tokenizer.h"

struct parser *create_parser()
{
    int *transition_qty = malloc(sizeof(int) * STATEQTY);                                                  // Cant de transiciones por cada nodo.
    struct parser_state_transition **states = malloc(sizeof(struct parser_state_transition *) * STATEQTY); // Lista de nodos

    // COMANDO

    transition_qty[COMMAND_STATE] = 3;
    struct parser_state_transition *command = malloc(sizeof(struct parser_state_transition) * 3); // Lista de transiciones del nodo
    states[COMMAND_STATE] = command;

    // Si hay un espacio, voy al primer argumento
    command[0].when = ' ';
    command[0].dest = FIRST_ARG_STATE;
    command[0].act1 = &restart_index;

    // Si hay un /r, termino
    command[1].when = '\r';
    command[1].dest = FINAL_STATE;
    command[1].act1 = NULL;

    // Si es cualquier caracter, lo escribo en el array de comandos
    command[2].when = ANY;
    command[2].dest = COMMAND_STATE;
    command[2].act1 = &store_command;

    // PRIMER ARGUMENTO

    transition_qty[FIRST_ARG_STATE] = 3;
    struct parser_state_transition *first_arg = malloc(sizeof(struct parser_state_transition) * 3); // Lista de transiciones del nodo
    states[FIRST_ARG_STATE] = first_arg;

    // Si hay un espacio, voy al segundo argumento
    first_arg[0].when = ' ';
    first_arg[0].dest = SECOND_ARG_STATE;
    first_arg[0].act1 = &restart_index;

    // Si hay un /r, termino
    first_arg[1].when = '\r';
    first_arg[1].dest = FINAL_STATE;
    first_arg[1].act1 = NULL;

    // Si es cualquier caracter, lo escribo en el array de comandos
    first_arg[2].when = ANY;
    first_arg[2].dest = FIRST_ARG_STATE;
    first_arg[2].act1 = &store_first_arg;

    // SEGUNDO ARGUMENTO

    transition_qty[SECOND_ARG_STATE] = 3;
    struct parser_state_transition *second_arg = malloc(sizeof(struct parser_state_transition) * 3); // Lista de transiciones del nodo
    states[SECOND_ARG_STATE] = second_arg;

    // Si hay un espacio, paso al estado de error (no puede haber mas de 2 args)
    second_arg[0].when = ' ';
    second_arg[0].dest = ERR0R_STATE;
    second_arg[0].act1 = NULL;

    // Si hay un /r, termino
    second_arg[1].when = '\r';
    second_arg[1].dest = FINAL_STATE;
    second_arg[1].act1 = NULL;

    // Si es cualquier caracter, lo escribo en el array de comandos
    second_arg[2].when = ANY;
    second_arg[2].dest = SECOND_ARG_STATE;
    second_arg[2].act1 = &store_second_arg;

    // ESTADO DE ERROR

    transition_qty[ERR0R_STATE] = 2;
    struct parser_state_transition *error = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[ERR0R_STATE] = error;

    // Si hay un /r, termino
    error[0].when = '\r';
    error[0].dest = FINAL_STATE;
    error[0].act1 = NULL;

    // Si es cualquier caracter, sigo en error
    error[1].when = ANY;
    error[1].dest = ERR0R_STATE;
    error[1].act1 = NULL;

    struct parser_definition parser_definition = {
        .start_state = COMMAND_STATE,
        .states = states,
        .states_count = STATEQTY,
        .states_n = transition_qty,
    };

    return parser_init(parser_no_classes(), &parser_definition);
}

void restart_index(struct parser_event *event, const uint8_t c)
{
    event->index = 0;
}

// MODULARIZAR!!!!!!!!!

void store_command(struct parser_event *event, const uint8_t c)
{
    if (event->commands[0] == NULL)
    {
        event->commands[0] = malloc(5 * sizeof(char)); // 4 bytes para command, y el null terminated
    }
    event->commands[0][(event->index)++] = c;
    event->commands[0][(event->index)] = '\0';
}

void store_first_arg(struct parser_event *event, const uint8_t c)
{
    if (event->commands[1] == NULL)
    {
        event->commands[1] = malloc(40 * sizeof(char)); // 40 para cada argumnento y null terminates
    }
    event->commands[1][(event->index)++] = c;
    event->commands[1][(event->index)] = '\0';
}

void store_second_arg(struct parser_event *event, const uint8_t c)
{
    if (event->commands[2] == NULL)
    {
        event->commands[2] = malloc(40 * sizeof(char)); // 40 para cada argumnento y null terminates
    }
    event->commands[2][(event->index)++] = c;
    event->commands[2][(event->index)] = '\0';
}