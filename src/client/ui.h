#ifndef UI_H
#define UI_H

#include "common/protocol.h"

#include <stddef.h>

#define UI_TEXT_MAX 256
#define UI_EVENTS_MAX 6
#define UI_INPUT_MAX 256
#define UI_SYMBOL_MAX 32
#define UI_MAP_MAX PROTO_MAX_LINE

/*
 * Stato completo della UI testuale.
 *
 * Il client non mantiene la logica autoritativa della partita: conserva solo
 * l'ultima rappresentazione ricevuta dal server e informazioni locali utili
 * al rendering, come eventi e contenuto del prompt.
 */
typedef struct {
    /* Flag di alto livello mostrati nel pannello stato. */
    int connected;
    int authenticated;
    int position_known;
    int game_over;

    /* Validita delle mappe: prima del login/aggiornamento i buffer sono vuoti. */
    int local_valid;   
    int global_valid;

    /* Dimensioni dichiarate dal server insieme alle mappe codificate. */
    int local_w;
    int local_h;
    int global_w;
    int global_h;

    /* Ultima posizione nota del giocatore autenticato. */
    int x;
    int y;

    /* Stringhe gia pronte per il rendering del terminale. */
    char server[UI_TEXT_MAX];
    char nickname[NICK_MAX + 1];
    char player_symbol[UI_SYMBOL_MAX];
    char status[UI_TEXT_MAX];
    char local_map[UI_MAP_MAX];
    char global_map[UI_MAP_MAX];
    char positions[PROTO_MAX_LINE];
    char game_result[UI_TEXT_MAX];

    /* Ring buffer degli ultimi eventi: event_next indica la prossima cella da sovrascrivere. */
    char events[UI_EVENTS_MAX][UI_TEXT_MAX];
    int event_count;
    int event_next;
} ui_state_t;

/* Inizializzazione e funzioni setter: separano parsing di rete e rendering. */
void ui_init(ui_state_t *ui, const char *host, const char *port);
void ui_set_connected(ui_state_t *ui, int connected);
void ui_set_user(ui_state_t *ui, const char *nickname, const char *player_symbol);
void ui_set_position(ui_state_t *ui, int x, int y);
void ui_set_local_map(ui_state_t *ui, int w, int h, const char *encoded);
void ui_set_global_map(ui_state_t *ui, int w, int h, const char *encoded, const char *positions);
void ui_set_positions(ui_state_t *ui, const char *positions);
void ui_set_game_over(ui_state_t *ui, const char *winner, const char *score, const char *scores);
void ui_add_event(ui_state_t *ui, const char *fmt, ...);

/* Ridisegna tutta la schermata usando lo stato corrente e il testo nel prompt. */
void ui_render(const ui_state_t *ui, const char *input);

/* Ripristina lo stato ANSI del terminale prima dell'uscita. */
void ui_finish(void);

#endif
