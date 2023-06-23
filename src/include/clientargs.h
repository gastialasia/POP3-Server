#ifndef CLIENTARGS_H
#define CLIENTARGS_H
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define DEFAULT_CONF_ADDR           "127.0.0.1"
#define DEFAULT_CONF_PORT           "1081"


#define DATA_SIZE                   65535
#define TOKEN_SIZE                  16
#define USERNAME_SIZE               5
#define PASSWORD_SIZE               16
#define MAX_LEN                     255

#define TOKEN_ENV_VAR_NAME          "MONITOR_TOKEN"

enum type {
    get     = 0,
    config  = 1
};

enum get_type {
    historic_connections    = 0,
    concurrent_connections  = 1,
    transferred_bytes       = 2,
};

enum config_type {
    config_maildir      = 0,
    config_buf_size     = 1,
    add_admin_user      = 2,
    del_admin_user      = 3,
    add_pop3_user       = 4,
    del_pop3_user       = 5,
};

union target {
    enum get_type get_type;
    enum config_type config_type;
};


struct add_admin_user {
    char        user[USERNAME_SIZE];
    char        finish_user;
    char        token[TOKEN_SIZE];
};

struct add_pop3_user {
    char        user[USERNAME_SIZE];
    char        separator;
    char        pass[PASSWORD_SIZE];
};

union data {
    uint8_t                         optional_data;          // To send 0 according to RFC
    char                            user[USERNAME_SIZE];
    char                            path[MAX_LEN];
     struct add_pop3_user    add_pop3_user_params;
    struct add_admin_user    add_admin_user_params;
};

struct client_request_args {
    enum type     method;
    union target    target;
    uint16_t        dlen;
    union data      data;
};

enum ip_version {
    ipv4 = 4,
    ipv6 = 6
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
size_t
parse_args(const int argc, char **argv, struct client_request_args *args, char *token, struct sockaddr_in *sin4, struct sockaddr_in6 *sin6, enum ip_version *ip_version);

#endif
