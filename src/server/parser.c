#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/parser.h"
#include "../include/tokenizer.h"

/* CDT del parser */
struct parser
{
    /** tipificación para cada caracter */
    const unsigned *classes;
    /** definición de estados */
    struct parser_definition *def;

    /* estado actual */
    unsigned state;

    /* evento que se retorna */
    struct parser_event e1;
};

void parser_destroy(struct parser *p)
{
    free_parser_events_rec(&p->e1); // No esta cambiando nada
    for (unsigned i = 0; i < p->def->states_count; i++)
    {
        free(p->def->states[i]);
    }
    free(p->def->states);
    free(p->def->states_n);
    free(p->def);
    if (p != NULL)
    {
        free(p);
    }
}

struct parser *parser_init(const unsigned *classes, struct parser_definition *def)
{
    struct parser *ret = malloc(sizeof(*ret));
    if (ret != NULL)
    {
        memset(ret, 0, sizeof(*ret));
        ret->classes = classes;
        ret->def = def;
        ret->state = def->start_state;
    }
    return ret;
}

void parser_reset(struct parser *p)
{
    p->state = p->def->start_state;
}

struct parser_event *
parser_feed(struct parser *p, const uint8_t c)
{
    // const unsigned type = p->classes[c];

    p->e1.next = 0;

    const struct parser_state_transition *state = p->def->states[p->state];
    const size_t n = p->def->states_n[p->state];
    bool matched = false;

    for (unsigned i = 0; i < n; i++)
    {
        const int when = state[i].when;

        if (state[i].when <= 0xFF)
        {
            matched = (c == when);
        }
        else if (state[i].when == ANY)
        {
            matched = true;
        }
        else
        {
            matched = false;
        }

        if (matched)
        {
            if (state[i].act1 != NULL)
            {
                state[i].act1(&p->e1, c);
            }
            p->state = state[i].dest;
            break;
        }
    }

    return &p->e1;
}

struct parser_event *get_last_event(struct parser *p)
{
    struct parser_event *aux = &p->e1;
    while (aux->next != NULL)
    {
        aux = aux->next;
    }
    return aux;
}

static const unsigned classes[0xFF] = {0x00};

const unsigned *
parser_no_classes(void)
{
    return classes;
}

void free_parser_events_rec(struct parser_event *pe)
{
    if (pe->next == NULL)
    {
        return;
    }
    free_parser_events_rec(pe->next);
    free(pe);
}
