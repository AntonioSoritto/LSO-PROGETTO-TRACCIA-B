#define _POSIX_C_SOURCE 200112L

#include "common/net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static char g_net_last_error[256] = "nessun errore di rete registrato";

// Salva un messaggio diagnostico globale semplice: il progetto e single-thread.
static void net_set_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(g_net_last_error, sizeof(g_net_last_error), fmt, ap);
    va_end(ap);
}

// Invia tutti i byte del buffer, gestendo invii parziali e interruzioni.
static int net_send_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n < 0)
        {
            // EINTR significa che un segnale ha interrotto send: si puo riprovare.
            if (errno == EINTR)
            {
                continue;
            }
            net_set_error("send su fd=%d fallita: %s", fd, strerror(errno));
            return -1;
        }
        if (n == 0)
        {
            net_set_error("send su fd=%d ha scritto 0 byte", fd);
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

// Invia una riga gia formata sul socket.
int net_send_line(int fd, const char *line)
{
    return net_send_all(fd, line, strlen(line));
}

const char *net_last_error(void)
{
    return g_net_last_error;
}

// Crea il socket TCP IPv4 del server, valida la porta e avvia listen().
int net_create_server_socket(const char *port)
{
    struct sockaddr_in addr;
    char *endptr;
    long port_num;
    int fd = -1;
    int yes = 1;

    // La porta arriva da argv come stringa: strtol permette di validarla completamente.
    errno = 0;
    port_num = strtol(port, &endptr, 10);
    if (errno != 0 || endptr == port || *endptr != '\0' ||
        port_num < 1 || port_num > 65535)
    {
        net_set_error("porta non valida: %s", port);
        return -1;
    }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        net_set_error("socket server fallita: %s", strerror(errno));
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
    {
        net_set_error("setsockopt(SO_REUSEADDR) fallita: %s", strerror(errno));
        close(fd);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    // INADDR_ANY accetta connessioni su tutte le interfacce della macchina.
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port_num);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        net_set_error("bind sulla porta %s fallita: %s", port, strerror(errno));
        close(fd);
        return -1;
    }
    if (listen(fd, 16) != 0)
    {
        net_set_error("listen sulla porta %s fallita: %s", port, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

// Apre una connessione TCP verso host:port usando getaddrinfo().
int net_connect_tcp(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *p;
    int fd = -1;
    int gai_rc;

    memset(&hints, 0, sizeof(hints));
    // AF_UNSPEC lascia a getaddrinfo la scelta fra IPv4 e IPv6.
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    gai_rc = getaddrinfo(host, port, &hints, &res);
    if (gai_rc != 0)
    {
        net_set_error("risoluzione indirizzo %s:%s fallita: %s", host, port, gai_strerror(gai_rc));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        // Si prova ogni indirizzo risolto finche una connect va a buon fine.
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
        {
            net_set_error("socket client fallita per %s:%s: %s", host, port, strerror(errno));
            continue;
        }
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
        net_set_error("connect verso %s:%s fallita: %s", host, port, strerror(errno));
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    if (fd < 0 && strcmp(g_net_last_error, "nessun errore di rete registrato") == 0)
    {
        net_set_error("nessun endpoint raggiungibile per %s:%s", host, port);
    }
    return fd;
}
