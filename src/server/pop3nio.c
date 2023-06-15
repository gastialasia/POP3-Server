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

#include "../include/pop3nio.h"
#include "../include/netutils.h"

#include "../include/parser.h"
#include "../include/tokenizer.h"
#include "../include/comparator.h"
#include "../include/email.h"

#define N(x) (sizeof(x) / sizeof((x)[0]))
#define BLOCK 5

// Variable globales
static unsigned int connections = 0; // live qty of connections
static struct pop3 *head_connection = NULL;
//path a la carpeta donde estan los directorios de todos los usuarios
char* path_to_maildir = INITIAL_PATH;

/*
 * Si bien cada estado tiene su propio struct que le da un alcance
 * acotado, disponemos de la siguiente estructura para hacer una única
 * alocación cuando recibimos la conexión.
 *
 * Se utiliza un contador de referencias (references) para saber cuando debemos
 * liberarlo finalmente, y un pool para reusar alocaciones previas.
 */

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

static void
auth_init(const unsigned state, struct selector_key *key)
{
    struct state_st *d = &ATTACHMENT(key)->state;
    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    d->parser = ATTACHMENT(key)->parser;
}

static void
trans_init(const unsigned state, struct selector_key *key)
{
    struct pop3* p3 = ATTACHMENT(key);
    load_mails(p3);
}

static unsigned client_read(struct selector_key *key)
{
    struct state_st *d = &ATTACHMENT(key)->state;
    unsigned int curr_state = ATTACHMENT(key)->stm.current->state;
    //bool error = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;
    fn_type command_handler;

    ptr = buffer_write_ptr(d->rb, &count); //Retorna un puntero en el que se puede escribir hasat nbytes
    n = recv(key->fd, ptr, count, 0);
    if (n > 0){
        buffer_write_adv(d->rb, n);    //Buffer: CAPA\r\n . CAPA\r\n
        while(buffer_can_read(d->rb)) {
            const uint8_t c = buffer_read(d->rb);
            parser_feed(d->parser, c);
            struct parser_event * pe = get_last_event(d->parser);
            //funcion para fijarnos si termino de pasear
            if (pe->complete) {
                if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE)){
                    command_handler = comparator(pe, curr_state); //Esto tiene que devolver el estado grande al que vamos.
                    curr_state = command_handler(d->wb, ATTACHMENT(key), pe->commands[1], pe->commands[2]);
                    restart_tokenizer(pe);
                } else {
                    curr_state = ERROR; //Si dio el selector
                }
            }
        }
    } else {
        curr_state = ERROR; // Si dio error el recv
    }

    //return error ? ERROR : curr_state;
    return curr_state;
}

static unsigned client_write(struct selector_key *key) { // key corresponde a un client_fd
    struct state_st *d = &ATTACHMENT(key)->state;

    unsigned ret = ATTACHMENT(key)->stm.current->state;
    uint8_t  *ptr;
    size_t   count;
    ssize_t  n;
    
    ptr = buffer_read_ptr(d->wb, &count);
    // esto deberia llamarse cuando el select lo despierta y sabe que se puede escribir al menos 1 byte, por eso no checkeamos el EWOULDBLOCK
    n = send(key->fd, ptr, count, MSG_NOSIGNAL);
    if (n == -1) {
        ret = ERROR;
    } else {
        buffer_read_adv(d->wb, n);
        // si terminamos de mandar toda la response del HELLO, hacemos transicion HELLO_WRITE -> AUTH_READ o HELLO_WRITE -> REQUEST_READ
        if (!buffer_can_read(d->wb)) {
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_READ)) {
                // en caso de que haya fallado el handshake del hello, el cliente es el que cerrara la conexion
                //ret = AUTH;//is_auth_on ? AUTH_READ : REQUEST_READ;
            } else {
                ret = ERROR;
            }
        }
    }

    return ret;
}



static void empty_function(const unsigned state, struct selector_key *key){
    return ;
}

static unsigned empty_function2(struct selector_key *key){
    return 1;
}

/*static void print_current_state(const unsigned state, struct selector_key *key){
    printf("LLegue a transaction\n");
}*/


/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state = AUTH,
        .on_arrival = auth_init, //Inicializar los parsers
        .on_read_ready = client_read, //Se hace la lectura
        .on_write_ready = client_write,//auth_write,
    },
    {
        .state = TRANSACTION,
        .on_arrival = trans_init,
        .on_departure = empty_function,
        .on_read_ready = client_read,
        .on_write_ready = client_write,
    },
    {
        .state = UPDATE,
        .on_arrival = empty_function,
        .on_departure = empty_function,
        .on_read_ready = empty_function2,
        .on_write_ready = empty_function2,
    },
    {
        .state = DONE,
        .on_arrival = empty_function,
        .on_departure = empty_function,
        .on_read_ready = empty_function2,
    },
    {
        .state = ERROR,
        .on_arrival = empty_function,
        .on_departure = empty_function,
        .on_read_ready = empty_function2,
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

    ret->stm.initial = AUTH;
    ret->stm.max_state = ERROR;
    ret->stm.states = pop3_describe_states();
    ret->credentials = malloc(sizeof(struct credentials_t));
    ret->parser = create_parser();
    ret->mails = malloc(BLOCK*sizeof(struct mail_t*));
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
