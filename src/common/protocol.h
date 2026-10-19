#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

/*
 * Modulo protocollo.
 *
 * Client e server comunicano con messaggi testuali terminati da '\n'.
 * Ogni messaggio e composto da token separati da spazi:
 *
 *   C2S_LOGIN mario password
 *   S2C_OK MOVED 10 4
 *
 * Questo header raccoglie dimensioni massime, tipi condivisi e funzioni
 * di utilita per non duplicare parsing e formattazione nei due programmi.
 */
#define PROTO_MAX_LINE 16384
#define PROTO_MAX_TOKENS 32
#define NICK_MAX 31
#define PASS_MAX 31

/* Direzioni logiche del gioco. Il protocollo le serializza come UP/DOWN/LEFT/RIGHT. */
typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} direction_t;

/* Valida nickname/password secondo le regole comuni a client e server. */
int proto_valid_name(const char *s, size_t max_len);

/* Traduce una stringa, anche abbreviata come w/a/s/d, in direction_t. */
int proto_parse_direction(const char *s, direction_t *dir);

/* Restituisce il nome canonico usato nei messaggi di rete. */
const char *proto_direction_name(direction_t dir);

/* Spezza una riga in token in-place: il buffer passato viene modificato. */
int proto_split(char *line, char **tokens, int max_tokens);

/* Costruisce una riga completa di newline finale controllando la capienza. */
int proto_make_line(char *dst, size_t dst_size, const char *fmt, ...);

#endif
