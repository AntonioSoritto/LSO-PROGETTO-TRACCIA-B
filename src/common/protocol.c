#include "common/protocol.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Valida nickname/password: non vuoti, lunghezza massima e caratteri ammessi.
int proto_valid_name(const char *s, size_t max_len) {
    size_t i;

    // NULL e stringa vuota vengono rifiutati per evitare account anonimi.
    if (s == NULL || s[0] == '\0') {
        return 0;
    }
    for (i = 0; s[i] != '\0'; ++i) {
        // i parte da 0: quando raggiunge max_len la stringa e troppo lunga.
        if (i >= max_len) {
            return 0;
        }
        // Si evitano spazi e simboli per mantenere semplice il protocollo a token.
        if (!isalnum((unsigned char)s[i]) && s[i] != '_' && s[i] != '-') {
            return 0;
        }
    }
    return 1;
}

// Converte testo o tasto rapido in una direzione del gioco.
int proto_parse_direction(const char *s, direction_t *dir) {
    // Accetta le forme usate dal protocollo e i tasti rapidi del client.
    if (s == NULL || dir == NULL) {
        return -1;
    }
    if (strcmp(s, "UP") == 0 || strcmp(s, "up") == 0 || strcmp(s, "w") == 0) {
        *dir = DIR_UP;
        return 0;
    }
    if (strcmp(s, "DOWN") == 0 || strcmp(s, "down") == 0 || strcmp(s, "s") == 0) {
        *dir = DIR_DOWN;
        return 0;
    }
    if (strcmp(s, "LEFT") == 0 || strcmp(s, "left") == 0 || strcmp(s, "a") == 0) {
        *dir = DIR_LEFT;
        return 0;
    }
    if (strcmp(s, "RIGHT") == 0 || strcmp(s, "right") == 0 || strcmp(s, "d") == 0) {
        *dir = DIR_RIGHT;
        return 0;
    }
    return -1;
}

// Restituisce il nome canonico usato nel protocollo per una direzione.
const char *proto_direction_name(direction_t dir) {
    switch (dir) {
        case DIR_UP: return "UP";
        case DIR_DOWN: return "DOWN";
        case DIR_LEFT: return "LEFT";
        case DIR_RIGHT: return "RIGHT";
        default: return "UNKNOWN";
    }
}

// Divide una riga in token separati da spazi, modificando il buffer originale.
int proto_split(char *line, char **tokens, int max_tokens) {
    int count = 0;
    char *p = line;

    while (*p != '\0' && count < max_tokens) {
        // Salta tutti i separatori iniziali o ripetuti.
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        // Il token inizia qui: si salva il puntatore direttamente dentro line.
        tokens[count++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            ++p;
        }
        if (*p != '\0') {
            // Sostituiamo il separatore con '\0', cosi ogni token diventa una stringa C.
            *p++ = '\0';
        }
    }
    return count;
}

// Costruisce una riga di protocollo e aggiunge sempre il newline finale.
int proto_make_line(char *dst, size_t dst_size, const char *fmt, ...) {
    va_list ap;
    int n;

    if (dst == NULL || dst_size == 0 || fmt == NULL) {
        return -1;
    }

    va_start(ap, fmt);
    n = vsnprintf(dst, dst_size, fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t)n + 2 > dst_size) {
        return -1;
    }
    // Il framing del protocollo dipende dal newline: senza '\n' il ricevitore resta in attesa.
    dst[n] = '\n';
    dst[n + 1] = '\0';
    return n + 1;
}
