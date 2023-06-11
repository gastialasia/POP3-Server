#ifndef PARSER_DECLARATION
#define PARSER_DECLARATION

// Los estados son los nodos del automata
enum states
{
    COMMAND_STATE,
    FIRST_ARG_STATE,
    SECOND_ARG_STATE,
    FINAL_STATE,
    ERR0R_STATE
};

#define STATEQTY 5

struct parser *create_parser();

void restart_index(struct parser_event *event, const uint8_t c);

void store_command(struct parser_event *event, const uint8_t c);

void store_first_arg(struct parser_event *event, const uint8_t c);

void store_second_arg(struct parser_event *event, const uint8_t c);

#endif