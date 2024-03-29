#ifndef SOCKS5NIO_H
#define SOCKS5NIO_H

#include <netdb.h>
#include "selector.h"
#include "stm.h"
#include "buffer.h"
#include "parser.h"

#define MAX_USERS 5
#define BUFFER_SIZE 1024*1024*4

struct client_t
{
    char *user;
    char *pass;
    struct client_t *next;
};

/** maquina de estados general */
enum pop3state
{
    AUTH,         // Se mueve a AUTH o ERROR
    TRANSACTION,  // Se mueve a QUIT, ERROR o TRANSACTION
    READING_MAIL, // for byte stuffing, se mueve a TRANSACTION o a ERROR
    WRITING_MAIL,
    UPDATE,
    // estados terminales
    DONE,
    ERROR,
};

struct credentials_t
{
    char *user;
    char *pass;
};

struct state_st
{
    buffer *rb, *wb;
    struct parser *parser;
};

struct mail_st
{
    buffer *rb, *wb;
    int mail_fd, socket_fd;
    int byte_stuffing_st;
    uint8_t done;
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

    struct credentials_t *credentials;

    // suma de la cantidad total de bytes entre todos los mails cargados en inbox
    size_t total_octates;

    size_t original_total_octates;

    unsigned mail_qty;

    unsigned max_index;

    unsigned selected_mail;

    int selected_mail_fd;

    struct mail_t **mails;

    uint8_t *dele_flags;

    struct parser *parser;

    struct pop3 *next;

    union
    {
        struct state_st state;
        struct mail_st mail;
    } client;
};

void pop3_passive_accept(struct selector_key *key);

void pop3_pool_destroy(void);

uint32_t get_historic();

uint32_t get_current();

size_t get_transfer_bytes();

struct client_t *unregister_clients_rec(struct client_t *c);

void unregister_clients(struct client_t *c);

int register_user(struct client_t *c, char *user, char *pass);

int unregister_user(struct client_t *c, char *user);

int validate_user(struct client_t *c, char *user, char *pass);

int change_buf_size(char *new_size);
#endif
