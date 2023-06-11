/**
 * socks5nio.c  - controla el flujo de un proxy SOCKSv5 (sockets no bloqueantes)
 */
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <assert.h> // assert
#include <errno.h>
#include <time.h>
#include <unistd.h> // close
#include <pthread.h>

#include <arpa/inet.h>

#include "../include/hello.h"
#include "../include/auth.h"
// #include "request.h"
#include "../include/buffer.h"

#include "../include/stm.h"
#include "../include/pop3nio.h"
#include "../include/netutils.h"

#define N(x) (sizeof(x) / sizeof((x)[0]))
#define BUFFER_SIZE 1024

////////////////////////////////////////////////////////////////////
// Definición de variables para cada estado

/** usado por HELLO_READ, HELLO_WRITE */
struct hello_st
{
    /** buffer utilizado para I/O */
    buffer *rb, *wb;
    struct hello_parser parser;
    /** el método de autenticación seleccionado */
    uint8_t method;
};

/** usado por HELLO_READ, HELLO_WRITE */
struct auth_st
{
    /** buffer utilizado para I/O */
    buffer *rb, *wb;
    struct auth_parser parser;
    char* user;
    char* pass;
};

//Prototipos
static unsigned int auth_process(const struct auth_st *d);

// Variable globales
static unsigned int connections = 0; // live qty of connections
static struct pop3 *head_connection = NULL;

/** maquina de estados general */
enum pop3state
{
    /**
     * recibe el mensaje `hello` del cliente, y lo procesa
     *
     * Intereses:
     *     - OP_READ sobre client_fd
     *
     * Transiciones:
     *   - HELLO_READ  mientras el mensaje no esté completo
     *   - HELLO_WRITE cuando está completo
     *   - ERROR       ante cualquier error (IO/parseo)
     */
    HELLO_READ,
     /*
     * envía la respuesta del `hello' al cliente.
     *
     * Intereses:
     *     - OP_WRITE sobre client_fd
     *
     * Transiciones:
     *   - HELLO_WRITE  mientras queden bytes por enviar
     *   - REQUEST_READ cuando se enviaron todos los bytes
     *   - ERROR        ante cualquier error (IO/parseo)
     */
    HELLO_WRITE,

    AUTH_READ,

    AUTH_WRITE,

    TRANSACTION_READ,

    TRANSACTION_WRITE,
    
    UPDATE_WRITE,
    
    // estados terminales
    DONE,
    ERROR,
};

/*
 * Si bien cada estado tiene su propio struct que le da un alcance
 * acotado, disponemos de la siguiente estructura para hacer una única
 * alocación cuando recibimos la conexión.
 *
 * Se utiliza un contador de referencias (references) para saber cuando debemos
 * liberarlo finalmente, y un pool para reusar alocaciones previas.
 */
struct pop3
{
    /** maquinas de estados */
    struct state_machine stm;

    int client_fd;
    struct sockaddr_storage client_addr; // direccion IP
    socklen_t client_addr_len;           // IPV4 or IPV6

    uint8_t raw_buff_a[BUFFER_SIZE], raw_buff_b[BUFFER_SIZE];
    buffer read_buffer, write_buffer;

    struct pop3 *next;

    /** estados para el client_fd */
    union
    {
        struct hello_st hello;
        struct auth_st auth;
    } client;
};

/** realmente destruye */
static void pop3_destroy(struct pop3 *s)
{
    //Liberar cada cliente
    //liberar bien todos los recursos
    free(s);
    connections--;
}

/** obtiene el struct (socks5 *) desde la llave de selección  */
#define ATTACHMENT(key) ((struct pop3 *)(key)->data)

///////////////////////////////////////////////////////////////////////////////
// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
static void pop3_done(struct selector_key *key);

/* declaración forward de los handlers de selección de una conexión
 * establecida entre un cliente y el proxy.
 */
static void pop3_read(struct selector_key *key);
static void pop3_write(struct selector_key *key);
static void pop3_block(struct selector_key *key);
static void pop3_close(struct selector_key *key);
static const struct fd_handler pop3_handler = {
    .handle_read = pop3_read,
    .handle_write = pop3_write,
    .handle_close = pop3_close,
    .handle_block = pop3_block,
};

/** inicializa las variables de los estados HELLO_… */
static void
hello_read_init(const unsigned state, struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    d->parser.data = &d->method;
    d->parser.on_authentication_method = on_hello_method, hello_parser_init(&d->parser);
}

/** inicializa las variables de los estados HELLO_… */
static void
auth_read_init(const unsigned state, struct selector_key *key)
{
    struct auth_st *d = &ATTACHMENT(key)->client.auth;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    auth_parser_init(&d->parser);
}

static void hello_salute(const unsigned state, struct selector_key *key){
    printf("estoy en la funcion de hello\n");
}

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
static unsigned hello_read(struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    unsigned ret = HELLO_READ;
    bool error = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;

    ptr = buffer_write_ptr(d->rb, &count);
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        buffer_write_adv(d->rb, n);
        const enum hello_state st = hello_consume(d->rb, &d->parser, &error);
        if (hello_is_done(st, 0))
        {
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {
                ret = hello_process(d);
            }
            else
            {
                ret = ERROR;
            }
        }
    }
    else
    {
        ret = ERROR;
    }

    return error ? ERROR : ret;
}

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
static unsigned auth_read(struct selector_key *key)
{
    struct auth_st *d = &ATTACHMENT(key)->client.auth;
    unsigned ret = AUTH_READ;
    bool error = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;

    ptr = buffer_write_ptr(d->rb, &count);
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        buffer_write_adv(d->rb, n);
        const enum auth_state st = auth_consume(d->rb, &d->parser, &error);
        if (auth_is_done(st, 0))
        {
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {
                ret = auth_process(d);
            }
            else
            {
                ret = ERROR;
            }
        }
    }
    else
    {
        ret = ERROR;
    }

    return error ? ERROR : ret;
}

/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state = HELLO_READ,
        .on_arrival = hello_read_init,
        .on_departure = hello_salute,
        .on_read_ready =  ,
    },
    {
        .state = HELLO_WRITE,
        .on_arrival = hello_read_init,
        .on_departure = hello_salute,
        .on_read_ready = hello_read,
    },
    {
        .state = DONE,
        .on_arrival = hello_read_init,
        .on_departure = hello_salute,
        .on_read_ready = hello_read,
    },
    {
        .state = ERROR,
        .on_arrival = hello_read_init,
        .on_departure = hello_salute,
        .on_read_ready = hello_read,
    },
};

static const struct state_definition *pop3_describe_states(void)
{
    return client_statbl;
}

static struct pop3 *pop3_new(int client_fd)
{
    struct pop3 *ret = calloc(1, sizeof(struct pop3));

    if (ret == NULL)
        goto finally;
    
    connections++;

    if (head_connection == NULL)
    {
        head_connection = ret;
    }
    else
    {
        struct pop3 *aux = head_connection;
        while (aux->next != NULL)
        {
            aux = aux->next;
        }
        aux->next = ret;
    }

    ret->client_fd = client_fd;
    ret->client_addr_len = sizeof(ret->client_addr);

    ret->stm.initial = HELLO_READ;
    ret->stm.max_state = ERROR;
    ret->stm.states = pop3_describe_states();
    stm_init(&ret->stm);

    buffer_init(&ret->read_buffer, N(ret->raw_buff_a), ret->raw_buff_a);
    buffer_init(&ret->write_buffer, N(ret->raw_buff_b), ret->raw_buff_b);

finally:
    return ret;
}

/** Intenta aceptar la nueva conexión entrante*/
void pop3_passive_accept(struct selector_key *key)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct pop3 *state = NULL;

    const int client = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client == -1)
    {
        goto fail;
    }
    if (selector_fd_set_nio(client) == -1)
    {
        goto fail;
    }
    state = pop3_new(client);
    if (state == NULL)
    {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&state->client_addr, &client_addr, client_addr_len);
    state->client_addr_len = client_addr_len;

    if (SELECTOR_SUCCESS != selector_register(key->s, client, &pop3_handler, OP_READ, state))
    {
        goto fail;
    }
    return;
fail:
    if (client != -1)
    {
        close(client);
    }
    //pop3_destroy(state);
}

////////////////////////////////////////////////////////////////////////////////
// HELLO
////////////////////////////////////////////////////////////////////////////////

/** callback del parser utilizado en `read_hello' */
static void
on_hello_method(struct hello_parser *p, const uint8_t method)
{
    uint8_t *selected = p->data;

    if (SOCKS_HELLO_NOAUTHENTICATION_REQUIRED == method)
    {
        *selected = method;
    }
}

static unsigned
hello_process(const struct hello_st *d);

/** procesamiento del mensaje `hello' */
static unsigned
hello_process(const struct hello_st *d)
{
    unsigned ret = HELLO_WRITE;

    uint8_t m = d->method;
    const uint8_t r = (m == SOCKS_HELLO_NO_ACCEPTABLE_METHODS) ? 0xFF : 0x00;
    if (-1 == hello_marshall(d->wb, r))
    {
        ret = ERROR;
    }
    if (SOCKS_HELLO_NO_ACCEPTABLE_METHODS == m)
    {
        ret = ERROR;
    }
    return ret;
}

static void
pop3_read(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum pop3state st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st)
    {
        pop3_done(key);
    }
}

static void
pop3_write(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum pop3state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st)
    {
        pop3_done(key);
    }
}

static void
pop3_block(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum pop3state st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st)
    {
        pop3_done(key);
    }
}

static void pop3_close(struct selector_key *key)
{
    pop3_destroy(ATTACHMENT(key));
}

static void
pop3_done(struct selector_key *key)
{
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
    };
    for (unsigned i = 0; i < N(fds); i++)
    {
        if (fds[i] != -1)
        {
            if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i]))
            {
                abort();
            }
            close(fds[i]);
        }
    }
}
