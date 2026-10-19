#ifndef SERVER_H
#define SERVER_H

#include "server/game.h"
#include "server/users.h"

#include <time.h>

/*
 * Stato di una connessione client.
 *
 * Una sessione puo esistere anche se non autenticata: in quel caso puo solo
 * registrarsi, fare login o chiudere. Dopo il login player_id collega la
 * sessione allo slot nello stato di gioco.
 */
typedef struct {
    int fd; 
    int authenticated; 
    int player_id;
    /* Buffer di accumulo per TCP stream: puo contenere mezza riga o piu righe. */
    char inbuf[PROTO_MAX_LINE];
    size_t inbuf_len;
} client_session_t;

/*
 * Stato globale del server.
 *
 * Il server e single-process e usa select(2), quindi mantiene tutte le sessioni
 * in un array e calcola manualmente timer di aggiornamento e fine partita.
 */
typedef struct {
    int listen_fd;
    int duration_sec;
    int period_sec;
    int running; 
    /* Timer assoluti basati su time(NULL). */
    time_t start_time;
    time_t next_update;
    /* Sottosistemi applicativi posseduti dal server. */
    user_db_t users;
    game_t game;
    /* Array dinamico di sessioni client. */
    client_session_t *clients;
    size_t client_count;
    size_t client_capacity;
} server_t;

/* Avvia il ciclo del server sulla porta indicata. */
int server_run(const char *port, int duration_sec, int period_sec);

#endif
