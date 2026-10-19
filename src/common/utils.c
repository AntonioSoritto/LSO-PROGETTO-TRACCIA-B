#include "common/utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

// Converte una stringa in long e valida che sia nel range richiesto.
long utils_parse_long(const char *s, long min_value, long max_value, int *ok)
{
    char *end = NULL;
    long value;

    // ok parte sempre da falso: viene portato a vero solo alla fine della validazione.
    if (ok != NULL)
    {
        *ok = 0;
    }
    if (s == NULL || *s == '\0')
    {
        return 0;
    }
    errno = 0;
    value = strtol(s, &end, 10);
    // Controlliamo: errore di conversione, nessuna cifra, caratteri residui e range.
    if (errno != 0 || end == s || *end != '\0' || value < min_value || value > max_value)
    {
        return 0;
    }
    if (ok != NULL)
    {
        *ok = 1;
    }
    return value;
}
