//Parser tokenizer: es responsable de consumir la entrada y separarla en un array [comando, argumento1, argumento2]

#include "../include/mail_tokenizer.h"
#include <stdlib.h>

#define CR_CHAR '\r'
#define LF_CHAR '\n'

struct parser *create_mail_parser()
{
    size_t *transition_qty = malloc(sizeof(size_t) * STATEQTY);                                                  // Cant de transiciones por cada nodo.
    struct parser_state_transition **states = malloc(sizeof(struct parser_state_transition *) * STATEQTY); // Lista de nodos

    // NEW_LINE

    transition_qty[NEW_LINE] = 2;
    struct parser_state_transition *new_line = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[NEW_LINE] = new_line;

    new_line[0].when = '.';
    new_line[0].dest = DOT;
    new_line[0].act1 = NULL; 

    new_line[1].when = ANY;
    new_line[1].dest = BYTE;
    new_line[1].act1 = NULL;

    // DOT

    transition_qty[DOT] = 2;
    struct parser_state_transition *dot = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[DOT] = dot;

    dot[0].when = CR_CHAR;
    dot[0].dest = DOT_CR;
    dot[0].act1 = NULL;

    dot[1].when = ANY;
    dot[1].dest = BYTE;
    dot[1].act1 = NULL;

    // DOT_CR

    transition_qty[DOT_CR] = 2;
    struct parser_state_transition *dot_cr = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[DOT_CR] = dot_cr;

    dot_cr[0].when = LF_CHAR;
    dot_cr[0].dest = FIN;
    dot_cr[0].act1 = NULL;

    dot_cr[1].when = ANY;
    dot_cr[1].dest = BYTE;
    dot_cr[1].act1 = NULL;

    // BYTE

    transition_qty[BYTE] = 2;
    struct parser_state_transition *byte = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[BYTE] = byte;

    byte[0].when = ANY;
    byte[0].dest = BYTE;
    byte[0].act1 = NULL;

    byte[1].when = CR_CHAR;
    byte[1].dest = CR;
    byte[1].act1 = NULL;

    // CR

    transition_qty[CR] = 2;
    struct parser_state_transition *cr = malloc(sizeof(struct parser_state_transition) * 2); // Lista de transiciones del nodo
    states[CR] = cr;

    cr[0].when = ANY;
    cr[0].dest = BYTE;
    cr[0].act1 = NULL;

    cr[1].when = LF_CHAR;
    cr[1].dest = NEW_LINE;
    cr[1].act1 = NULL;

    //Creacion del parser_definition

    struct parser_definition * parser_definition = malloc(sizeof(struct parser_definition));

    parser_definition->start_state = NEW_LINE;
    parser_definition->states = states;
    parser_definition->states_count = STATEQTY;
    parser_definition->states_n = transition_qty;

    return parser_init(parser_no_classes(), parser_definition);
}