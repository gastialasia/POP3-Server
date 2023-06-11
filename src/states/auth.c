#include "../include/auth.h"

extern void auth_parser_init(struct auth_parser *p)
{
    p->parser = create_parser();
    p->state = auth_no_user;
}

extern enum auth_state auth_parser_feed(struct auth_parser *p, const uint8_t c)
{
    switch (p->state)
    {
    case auth_no_user:
        
        break;
    default:
        fprintf(stderr, "unknown state %d\n", p->state);
        abort();
    }

    return p->state;
}

extern bool auth_is_done(const enum auth_state state, bool *errored)
{
    bool ret;
    switch (state)
    {
    case auth_error_invalid:
        if (0 != errored)
        {
            *errored = true;
        }
        /* no break */
    case auth_done:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

extern const char * auth_error(const struct auth_parser *p)
{
    char *ret;
    switch (p->state)
    {
    case auth_error_invalid:
        ret = "unsupported version";
        break;
    default:
        ret = "";
        break;
    }
    return ret;
}

extern void auth_parser_close(struct auth_parser *p)
{
    /* no hay nada que liberar */
}

extern enum auth_state auth_consume(buffer *b, struct auth_parser *p, bool *errored)
{
    enum auth_state st = p->state;

    while (buffer_can_read(b))
    {
        const uint8_t c = buffer_read(b);
        st = auth_parser_feed(p, c);
        if (auth_is_done(st, errored))
        {
            break;
        }
    }
    return st;
}

extern int auth_marshall(buffer *b, const uint8_t method)
{
    size_t n;
    uint8_t *buff = buffer_write_ptr(b, &n);
    if (n < 2)
    {
        return -1;
    }
    buff[0] = 0x05;
    buffer_write_adv(b, 2);
    return 2;
}