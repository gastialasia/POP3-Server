#ifndef SOCKS5NIO_H
#define SOCKS5NIO_H

#include <netdb.h>
#include "selector.h"
#include "stm.h"
#include "buffer.h"

#define MAX_USERS 5
#define BUFFER_SIZE 1024

/** maquina de estados general */
enum pop3state
{
    AUTH_NO_USER, //Se mueve a AUTH_NO_USER, AUTH_NO_PASS o ERROR
    AUTH_NO_PASS, // Se mueve a TRANSACTION, AUTH_NO_PASS o ERROR
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

struct auth_st
{
    /** buffer utilizado para I/O */
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

    struct pop3 *next;

    /** estados para el client_fd */
    union
    {
        struct auth_st auth_no_user;
        struct auth_st auth_no_pass;
    } client;
};



void pop3_passive_accept(struct selector_key *key);

void pop3_pool_destroy(void);

#endif 