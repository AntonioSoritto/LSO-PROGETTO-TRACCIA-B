#ifndef GAME_H
#define GAME_H

#include "common/protocol.h"

#define MAP_W 30
#define MAP_H 15
#define LOCAL_VIEW_W 11
#define LOCAL_VIEW_H 11
#define PLAYER_SYMBOL_MAX 32

/*
 * Modello della partita.
 *
 * La griglia e autoritativa solo sul server. Ogni cella puo essere:
 * - muro in wall[y][x], non conquistabile;
 * - libera o posseduta da uno slot giocatore in owner[y][x];
 * - occupata temporaneamente da un giocatore attivo tramite player_t.x/y.
 */

// Stato persistente di un giocatore. Lo slot resta associato al nickname
// anche dopo la disconnessione, cosi la proprieta delle celle rimane stabile.
typedef struct {
    char nickname[NICK_MAX + 1];
    /* active indica presenza online; used indica che lo slot e stato assegnato almeno una volta. */
    int active;
    int used;
    char symbol[PLAYER_SYMBOL_MAX];
    /* Coordinate correnti sulla griglia globale. */
    int x;
    int y;
    /* Muri scoperti dal singolo giocatore: la conoscenza e individuale. */
    unsigned char discovered_walls[MAP_H][MAP_W];
} player_t;

typedef struct {
    /* wall vale 1 per muro fisico, 0 per cella attraversabile. */
    int wall[MAP_H][MAP_W];
    /* owner contiene l'indice player_t proprietario, oppure -1 se non conquistata. */
    int owner[MAP_H][MAP_W];
    /* Array dinamico di slot giocatore. */
    player_t *players;
    size_t player_count;
    size_t player_capacity;
} game_t;

/* Lifecycle dello stato di gioco. */
void game_init(game_t *game);
void game_free(game_t *game);

/* Gestione presenza giocatori. */
int game_add_player(game_t *game, const char *nickname);
void game_remove_player(game_t *game, int player_id);
int game_find_player(const game_t *game, const char *nickname);

/* Applica un movimento: 0 ok, valori negativi distinguono i casi di errore. */
int game_move(game_t *game, int player_id, direction_t dir);

/* Serializzazioni usate dal protocollo server->client. */
int game_build_local_map(const game_t *game, int player_id, char *out, size_t out_size);
int game_build_global_map(const game_t *game, char *out, size_t out_size);
int game_build_positions(const game_t *game, char *out, size_t out_size);
int game_build_scores(const game_t *game, char *out, size_t out_size);

/* Calcolo finale del vincitore in base al numero di celle conquistate. */
int game_winner(const game_t *game, char *nickname, size_t nickname_size, int *score);

#endif
