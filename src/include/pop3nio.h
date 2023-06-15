#ifndef SOCKS5NIO_H
#define SOCKS5NIO_H

#include <netdb.h>
#include "selector.h"
#include "stm.h"
#include "buffer.h"
#include "parser.h"
#include "array.h"

#define INITIAL_PATH "./directories/"

#define MAX_USERS 5
#define BUFFER_SIZE 1024

/** maquina de estados general */
enum pop3state
{
    AUTH, //Se mueve a AUTH o ERROR
    TRANSACTION, // Se mueve a QUIT, ERROR o TRANSACTION
    UPDATE,
    // estados terminales
    DONE,
    ERROR,
};

struct credentials_t {
    char * user;
    char * pass;
};

struct state_st{
    buffer *rb, *wb;
    struct parser * parser;
};

struct pop3
{
    /** maquinas de estados */
    struct state_machine stm;

    int client_fd;
    struct sockaddr_storage client_addr; // direccion IP
    socklen_t client_addr_len;           // IPV4 or IPV6

    uint8_t raw_buff_a[BUFFER_SIZE], raw_buff_b[BUFFER_SIZE];
    buffer read_buffer, write_buffer;

    struct credentials_t * credentials;

    mail_array inbox;

    struct parser * parser;

    struct state_st state;

    struct pop3 *next;
};

void pop3_passive_accept(struct selector_key *key);

void pop3_pool_destroy(void);

#endif 