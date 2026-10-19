#ifndef USERS_H
#define USERS_H

#include "common/protocol.h"

/* Credenziali in memoria. Per un progetto reale la password non andrebbe salvata in chiaro. */
typedef struct {
    char nickname[NICK_MAX + 1];
    char password[PASS_MAX + 1];
} user_t;

/* Database dinamico minimale: array, numero di elementi usati e capacita allocata. */
typedef struct {
    user_t *items;
    size_t count;
    size_t capacity;
} user_db_t;

/* Libera l'array degli utenti e azzera la struttura. */
void users_free(user_db_t *db);

/* Registra un utente: 0 ok, -1 esiste, -2 credenziali non valide, -3 memoria. */
int users_register(user_db_t *db, const char *nickname, const char *password);

/* Verifica nickname/password: 0 se corrispondono, -1 altrimenti. */
int users_authenticate(const user_db_t *db, const char *nickname, const char *password);

#endif
