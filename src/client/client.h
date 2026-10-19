#ifndef CLIENT_H
#define CLIENT_H

/*
 * Avvia il client interattivo.
 *
 * host e port identificano il server TCP. La funzione ritorna 0 in caso di
 * chiusura normale, -1 per errori di rete o di I/O.
 */
int client_run(const char *host, const char *port);

#endif
