/**
 * server.c - servidor proxy socks concurrente
 *
 * Interpreta los argumentos de línea de comandos, y monta un socket
 * pasivo.
 *
 * Todas las conexiones entrantes se manejarán en éste hilo.
 *
 * Se descargará en otro hilos las operaciones bloqueantes (resolución de
 * DNS utilizando getaddrinfo), pero toda esa complejidad está oculta en
 * el selector.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include<time.h>

#include "include/selector.h"
#include "include/pop3nio.h"
#include "include/monitornio.h"
#include "include/args.h"
#include "include/email.h"

#define MAX_SOCKS 500
static const int FD_UNUSED = -1;
#define IS_FD_USED(fd) ((FD_UNUSED != fd))
static bool done = false;
// clientes
//  struct client_t c { .user="foo", .pass="bar", .next=NULL };
struct client_t *clients = NULL;

static void
sigterm_handler(const int signal)
{
    printf("Signal %d, cleaning up and exiting\n", signal);
    done = true;
}

static int bind_ipv4_socket(int bind_address, unsigned port);
static int bind_ipv6_socket(struct in6_addr bind_address, unsigned port);

int main(const int argc, char **argv)
{

    struct pop3args args;
    parse_args(argc, argv, &args);
    printf("%s\n",args.path);
    change_maildir(args.path);


    const char *err_msg = NULL;
    selector_status ss = SELECTOR_SUCCESS;
    fd_selector selector = NULL;

    int server_v4 = FD_UNUSED, monitor_v4 = FD_UNUSED;

    struct in6_addr server_ipv6_addr = in6addr_any, monitor_ipv6_addr = in6addr_any;
    int server_v6 = FD_UNUSED, monitor_v6 = FD_UNUSED;

    server_v4 = bind_ipv4_socket(INADDR_ANY, args.port);
    if (server_v4 < 0)
    {
        err_msg = "unable to create IPv4 server socket";
        goto finally;
    }
    fprintf(stdout, "Server: listening on IPv4 TCP port %d\n", args.port);

    monitor_v4 = bind_ipv4_socket(INADDR_ANY, 1081);
    if (monitor_v4 < 0)
    {
        err_msg = "unable to create IPv4 monitor socket";
        goto finally;
    }
    fprintf(stdout, "Monitor: listening on IPv4 TCP port %d\n", 1081);

    server_v6 = bind_ipv6_socket(server_ipv6_addr, args.port);
    if (server_v6 < 0)
    {
        err_msg = "unable to create IPv6 socket";
        goto finally;
    }
    fprintf(stdout, "Server: listening on IPv6 TCP port %d\n", args.port);

    monitor_v6 = bind_ipv6_socket(monitor_ipv6_addr, 1081);
    if (monitor_v6 < 0)
    {
        err_msg = "unable to create IPv6 socket";
        goto finally;
    }
    fprintf(stdout, "Monitor: listening on IPv6 TCP port %d\n", 1081);

    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
    if (IS_FD_USED(server_v4) && (selector_fd_set_nio(server_v4) == -1))
    {
        err_msg = "Getting pop3 server ipv4 socket flags";
        goto finally;
    }

    if (IS_FD_USED(server_v6) && (selector_fd_set_nio(server_v6) == -1))
    {
        err_msg = "getting pop3 server ipv6 socket flags";
        goto finally;
    }

    if (IS_FD_USED(monitor_v4) && (selector_fd_set_nio(monitor_v4) == -1))
    {
        err_msg = "getting monitor server ipv4 socket flags";
        goto finally;
    }

    if (IS_FD_USED(monitor_v6) && (selector_fd_set_nio(monitor_v6) == -1))
    {
        err_msg = "Getting monitor server ipv6 socket flags";
        goto finally;
    }

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };
    if (0 != selector_init(&conf))
    {
        err_msg = "initializing selector";
        goto finally;
    }

    selector = selector_new(MAX_SOCKS);
    if (selector == NULL)
    {
        err_msg = "unable to create selector";
        goto finally;
    }

    const struct fd_handler pop3 = {
        .handle_read = pop3_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL, // nada que liberar
    };

    if (IS_FD_USED(server_v4))
    {
        ss = selector_register(selector, server_v4, &pop3, OP_READ, NULL);
        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "registering IPv4 socks fd";
            goto finally;
        }
    }
    if (IS_FD_USED(server_v6))
    {
        ss = selector_register(selector, server_v6, &pop3, OP_READ, NULL);
        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "registering IPv6 socks fd";
            goto finally;
        }
    }
    const struct fd_handler monitor = {
        .handle_read = monitor_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    if (IS_FD_USED(monitor_v4))
    {
        ss = selector_register(selector, monitor_v4, &monitor, OP_READ, NULL);
        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "registering IPv4 monitor fd";
            goto finally;
        }
    }
    if (IS_FD_USED(monitor_v6))
    {
        ss = selector_register(selector, monitor_v6, &monitor, OP_READ, NULL);
        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "registering IPv6 monitor fd";
            goto finally;
        }
    }
    register_user(clients, "foo", "bar");
    add_new_admin("root", "123456789abcdef1");

    /* ss = selector_register(selector, server, &pop3, OP_READ, NULL);*/
    if (ss != SELECTOR_SUCCESS)
    {
        err_msg = "registering fd";
        goto finally;
    }

    
    for (; !done;)
    {
        err_msg = NULL;
        ss = selector_select(selector);

        if (ss != SELECTOR_SUCCESS)
        {
            err_msg = "serving";
            goto finally;
        }
    }
    if (err_msg == NULL)
    {
        err_msg = "closing";
    }

    int ret = 0;
finally:
    if (ss != SELECTOR_SUCCESS)
    {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "" : err_msg,
                ss == SELECTOR_IO
                    ? strerror(errno)
                    : selector_error(ss));
        ret = 2;
    }
    else if (err_msg)
    {
        perror(err_msg);
        ret = 1;
    }
    if (selector != NULL)
    {
        selector_destroy(selector);
    }
    selector_close();
    admin_connection_pool_destroy();
    unregister_clients(clients);
    if (server_v4 >= 0)
        close(server_v4);
    if (server_v6 >= 0)
        close(server_v6);
    if (monitor_v4 >= 0)
        close(monitor_v4);
    if (monitor_v6 >= 0)
        close(monitor_v6);
    return ret;
}

static int
create_socket(sa_family_t family)
{
    const int s = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
    {
        fprintf(stderr, "Unable to create socket\n");
        return -1;
    }

    int sock_optval[] = {1}; //  valor de SO_REUSEADDR
    socklen_t sock_optlen = sizeof(int);
    // man 7 ip. no importa reportar nada si falla.
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)sock_optval, sock_optlen);
    return s;
}

static int
bind_socket(int server, struct sockaddr *address, socklen_t address_len)
{
    if (bind(server, address, address_len) < 0)
    {
        fprintf(stderr, "Unable to bind socket\n");
        return -1;
    }

    if (listen(server, 20) < 0)
    {
        fprintf(stderr, "Unable to listen on socket\n");
        return -1;
    }

    return 0;
}

static int
bind_ipv4_socket(int bind_address, unsigned port)
{
    const int server = create_socket(AF_INET);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(bind_address);
    addr.sin_port = htons(port); // htons translates a short integer from host byte order to network byte order.

    if (bind_socket(server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        printf("Error in bind\n");
        return -1;
    }
    return server;
}

static int
bind_ipv6_socket(struct in6_addr bind_address, unsigned port)
{
    const int server = create_socket(AF_INET6);
    setsockopt(server, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int)); // man ipv6, si falla fallara el bind

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = bind_address;
    addr.sin6_port = htons(port); // htons translates a short integer from host byte order to network byte order.

    if (bind_socket(server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        return -1;

    return server;
}
