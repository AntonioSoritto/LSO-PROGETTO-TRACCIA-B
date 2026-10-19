#ifndef NET_H
#define NET_H

#include <stddef.h>

/*
 * Interfaccia di rete comune.
 *
 * Il progetto usa TCP stream: non esiste un confine naturale fra messaggi,
 * quindi i moduli applicativi inviano righe testuali complete e poi accumulano
 * byte in ricezione finche non trovano '\n'.
 */

/* Crea, configura, associa e mette in ascolto un socket server TCP. */
int net_create_server_socket(const char *port);

/* Apre una connessione TCP verso host:port, supportando IPv4/IPv6 via getaddrinfo. */
int net_connect_tcp(const char *host, const char *port);

/* Invia l'intera riga gia formata; gestisce eventuali send parziali. */
int net_send_line(int fd, const char *line);

/* Ultimo errore di rete in forma leggibile, utile per main e diagnostica. */
const char *net_last_error(void);

#endif
