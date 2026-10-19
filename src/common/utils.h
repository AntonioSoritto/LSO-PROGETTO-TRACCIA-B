#ifndef UTILS_H
#define UTILS_H

/* Converte una stringa numerica in long e segnala con ok se e valida e nel range. */
long utils_parse_long(const char *s, long min_value, long max_value, int *ok);

#endif
