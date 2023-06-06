#ifndef SOCKS5NIO_H
#define SOCKS5NIO_H

#include <netdb.h>
#include "selector.h"

#define MAX_USERS 5

void pop3_passive_accept(struct selector_key *key);

void pop3_pool_destroy(void);

#endif 