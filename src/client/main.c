#include "client/client.h"
#include "common/net.h"

#include <stdio.h>
#include <stdlib.h>

// Punto di ingresso del client: richiede host/porta e avvia client_run().
int main(int argc, char **argv) {
    // Il client ha bisogno sia dell'indirizzo sia della porta del server.
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <host> <porta>\n", argv[0]);
        return EXIT_FAILURE;
    }
    // Gli errori di rete vengono esposti dal modulo common/net.
    if (client_run(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Client terminato con errore: %s\n", net_last_error());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
