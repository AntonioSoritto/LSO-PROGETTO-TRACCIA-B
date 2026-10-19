#include "server/users.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_USERS_CAPACITY 16

// Libera il database utenti mantenuto in memoria.
void users_free(user_db_t *db) {
    free(db->items);
    memset(db, 0, sizeof(*db));
}

// Garantisce spazio per almeno needed utenti nell'array dinamico.
static int users_reserve(user_db_t *db, size_t needed) {
    user_t *new_items;
    size_t new_capacity = db->capacity == 0 ? INITIAL_USERS_CAPACITY : db->capacity;

    // Crescita geometrica: poche realloc anche con molti utenti.
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    if (new_capacity == db->capacity) {
        return 0;
    }
    new_items = realloc(db->items, new_capacity * sizeof(*new_items));
    if (new_items == NULL) {
        return -1;
    }
    db->items = new_items;
    db->capacity = new_capacity;
    return 0;
}

// Controlla se un nickname e gia registrato.
static int users_exists(const user_db_t *db, const char *nickname) {
    size_t i;
    for (i = 0; i < db->count; ++i) {
        if (strcmp(db->items[i].nickname, nickname) == 0) {
            return 1;
        }
    }
    return 0;
}

// Registra un nuovo utente dopo validazione di nickname e password.
int users_register(user_db_t *db, const char *nickname, const char *password) {
    // La validazione e comune al protocollo: niente spazi, cosi il parsing a token resta sicuro.
    if (!proto_valid_name(nickname, NICK_MAX) || !proto_valid_name(password, PASS_MAX)) {
        return -2;
    }
    if (users_exists(db, nickname)) {
        return -1;
    }
    if (users_reserve(db, db->count + 1) != 0) {
        return -3;
    }
    // strncpy viene completata con terminatore esplicito per garantire stringhe C valide.
    strncpy(db->items[db->count].nickname, nickname, NICK_MAX);
    strncpy(db->items[db->count].password, password, PASS_MAX);
    db->items[db->count].nickname[NICK_MAX] = '\0';
    db->items[db->count].password[PASS_MAX] = '\0';
    db->count++;
    return 0;
}

// Verifica che nickname e password corrispondano a un utente registrato.
int users_authenticate(const user_db_t *db, const char *nickname, const char *password) {
    size_t i;
    for (i = 0; i < db->count; ++i) {
        if (strcmp(db->items[i].nickname, nickname) == 0 &&
            strcmp(db->items[i].password, password) == 0) {
            return 0;
        }
    }
    return -1;
}
