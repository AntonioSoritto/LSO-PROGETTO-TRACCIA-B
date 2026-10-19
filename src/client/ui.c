#include "client/ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_MAP_ROWS 30
#define MAX_ROW_LEN 256
#define MAX_PLAYERS 64

typedef struct {
    char symbol[8];
    int x;
    int y;
} player_pos_t;

static void safe_copy(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) return;
    if (src == NULL) src = "";
    snprintf(dst, dst_size, "%s", src);
}

static void print_divider(void)
{
    puts("===================================================================================================================================");
}

static void print_thin_divider(void)
{
    puts("-----------------------------------------------------------------------------------------------------------------------------------");
}

/* Hash per distribuire in modo pseudo-casuale e uniforme i colori ANSI a 256 tonalità */
static unsigned int hash_text(const char *s)
{
    unsigned int h = 5381;
    while (s != NULL && *s != '\0')
    {
        h = ((h << 5) + h) ^ (unsigned char)*s;
        ++s;
    }
    return h;
}

/* Genera dinamicamente un codice colore ANSI a 256 colori visibile e brillante */
static void get_player_color_code(const char *symbol, char *out_color, size_t out_size)
{
    if (symbol == NULL || symbol[0] == '\0')
    {
        snprintf(out_color, out_size, "\033[1;37m"); /* Bianco di default */
        return;
    }

    /* Lista di 30 colori vivaci ed estesi scelti dalla palette a 256 colori ANSI per evitare toni troppo scuri o neri */
    static const int ansi_256_palette[] = {
        10, 11, 12, 13, 14, 9, 202, 208, 214, 220, 226, 190, 154, 118, 82, 46,
        51, 45, 39, 27, 129, 165, 201, 207, 198, 162, 126, 172, 178, 184
    };

    unsigned int idx = hash_text(symbol) % (sizeof(ansi_256_palette) / sizeof(ansi_256_palette[0]));
    snprintf(out_color, out_size, "\033[38;5;%dm", ansi_256_palette[idx]);
}

/* Parsing posizioni giocatori inviate dal server */
static int parse_player_positions(const char *positions_str, player_pos_t players[], int max_p)
{
    if (positions_str == NULL || positions_str[0] == '\0' || strcmp(positions_str, "-") == 0)
    {
        return 0;
    }

    char buf[2048];
    strncpy(buf, positions_str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    int count = 0;
    char *saveptr_token;
    char *token = strtok_r(buf, " ;,", &saveptr_token);

    while (token != NULL && count < max_p)
    {
        char token_copy[128];
        strncpy(token_copy, token, sizeof(token_copy) - 1);
        token_copy[sizeof(token_copy) - 1] = '\0';

        char *parts[5];
        int nparts = 0;
        char *saveptr_sub;
        char *sub = strtok_r(token_copy, ":", &saveptr_sub);

        while (sub != NULL && nparts < 5)
        {
            parts[nparts++] = sub;
            sub = strtok_r(NULL, ":", &saveptr_sub);
        }

        if (nparts >= 4)
        {
            strncpy(players[count].symbol, parts[1], sizeof(players[count].symbol) - 1);
            players[count].x = atoi(parts[2]);
            players[count].y = atoi(parts[3]);
            count++;
        }
        else if (nparts == 3)
        {
            strncpy(players[count].symbol, parts[0], sizeof(players[count].symbol) - 1);
            players[count].x = atoi(parts[1]);
            players[count].y = atoi(parts[2]);
            count++;
        }

        token = strtok_r(NULL, " ;,", &saveptr_token);
    }

    return count;
}

static const char *get_player_at(int x, int y, const player_pos_t players[], int num_players)
{
    for (int i = 0; i < num_players; ++i)
    {
        if (players[i].x == x && players[i].y == y)
        {
            return players[i].symbol;
        }
    }
    return NULL;
}

/* Rendering Mappa Locale con colore dinamico a 256 toni */
static void print_local_map_row(const char *raw_row, const char *my_symbol)
{
    int cells_printed = 0;
    char col[32];

    if (raw_row != NULL && raw_row[0] != '\0')
    {
        char row_copy[512];
        strncpy(row_copy, raw_row, sizeof(row_copy) - 1);
        row_copy[sizeof(row_copy) - 1] = '\0';

        char *saveptr;
        char *cell = strtok_r(row_copy, ",", &saveptr);
        while (cell != NULL && cells_printed < 11)
        {
            if (strcmp(cell, "@") == 0)
            {
                get_player_color_code(my_symbol, col, sizeof(col));
                printf("%s@  \033[0m", col);
            }
            else if (strcmp(cell, "#") == 0)
            {
                printf("\033[1;37m█  \033[0m");       /* Ostacolo */
            }
            else if (strcmp(cell, "~") == 0)
            {
                printf("\033[0;34m~  \033[0m");       /* Fuori confini */
            }
            else if (strcmp(cell, ".") == 0)
            {
                printf("\033[0;30m.  \033[0m");       /* Cella vuota */
            }
            else if (strncmp(cell, "P", 1) == 0)
            {
                get_player_color_code(cell, col, sizeof(col));
                printf("%s%-2s \033[0m", col, cell);
            }
            else
            {
                printf("%-2s ", cell);
            }
            cells_printed++;
            cell = strtok_r(NULL, ",", &saveptr);
        }
    }

    while (cells_printed < 11)
    {
        printf("   ");
        cells_printed++;
    }
}

/* Rendering Mappa Globale con colori dinamici a 256 toni per ogni giocatore */
static void print_global_map_row(const char *raw_row, int row_idx, const player_pos_t players[], int num_players)
{
    if (raw_row == NULL || raw_row[0] == '\0')
    {
        putchar('\n');
        return;
    }

    char row_copy[512];
    strncpy(row_copy, raw_row, sizeof(row_copy) - 1);
    row_copy[sizeof(row_copy) - 1] = '\0';

    int col_idx = 0;
    char col[32];
    char *saveptr;
    char *cell = strtok_r(row_copy, ",", &saveptr);
    while (cell != NULL)
    {
        const char *p_at = get_player_at(col_idx, row_idx, players, num_players);

        if (p_at != NULL)
        {
            get_player_color_code(p_at, col, sizeof(col));
            printf("%s@  \033[0m", col);
        }
        else if (strcmp(cell, ".") == 0)
        {
            printf("\033[0;30m.  \033[0m");          /* Cella vuota */
        }
        else if (strncmp(cell, "P", 1) == 0)
        {
            get_player_color_code(cell, col, sizeof(col));
            printf("%s%-2s \033[0m", col, cell);
        }
        else
        {
            printf("%-2s ", cell);
        }

        col_idx++;
        cell = strtok_r(NULL, ",", &saveptr);
    }
    putchar('\n');
}

static int parse_map_rows(const char *encoded, char rows[MAX_MAP_ROWS][MAX_ROW_LEN])
{
    if (encoded == NULL || encoded[0] == '\0') return 0;

    char copy[16384];
    strncpy(copy, encoded, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    int count = 0;
    char *saveptr;
    char *line = strtok_r(copy, "/", &saveptr);
    while (line != NULL && count < MAX_MAP_ROWS)
    {
        strncpy(rows[count], line, MAX_ROW_LEN - 1);
        rows[count][MAX_ROW_LEN - 1] = '\0';
        count++;
        line = strtok_r(NULL, "/", &saveptr);
    }
    return count;
}

void ui_init(ui_state_t *ui, const char *host, const char *port)
{
    memset(ui, 0, sizeof(*ui));
    snprintf(ui->server, sizeof(ui->server), "%s:%s", host, port);
    safe_copy(ui->nickname, sizeof(ui->nickname), "-");
    safe_copy(ui->positions, sizeof(ui->positions), "-");
    safe_copy(ui->status, sizeof(ui->status), "connessione in corso");
    safe_copy(ui->player_symbol, sizeof(ui->player_symbol), "P0");
}

void ui_set_connected(ui_state_t *ui, int connected)
{
    ui->connected = connected;
    safe_copy(ui->status, sizeof(ui->status), connected ? "connesso" : "disconnesso");
}

void ui_set_user(ui_state_t *ui, const char *nickname, const char *player_symbol)
{
    ui->authenticated = 1;
    safe_copy(ui->nickname, sizeof(ui->nickname), nickname);
    safe_copy(ui->player_symbol, sizeof(ui->player_symbol), player_symbol);
    safe_copy(ui->status, sizeof(ui->status), "autenticato");
}

void ui_set_position(ui_state_t *ui, int x, int y)
{
    ui->position_known = 1;
    ui->x = x;
    ui->y = y;
}

void ui_set_local_map(ui_state_t *ui, int w, int h, const char *encoded)
{
    ui->local_valid = 1;
    ui->local_w = w;
    ui->local_h = h;
    safe_copy(ui->local_map, sizeof(ui->local_map), encoded);
}

void ui_set_global_map(ui_state_t *ui, int w, int h, const char *encoded, const char *positions)
{
    ui->global_valid = 1;
    ui->global_w = w;
    ui->global_h = h;
    safe_copy(ui->global_map, sizeof(ui->global_map), encoded);
    safe_copy(ui->positions, sizeof(ui->positions), positions);
}

void ui_set_positions(ui_state_t *ui, const char *positions)
{
    safe_copy(ui->positions, sizeof(ui->positions), positions);
}

void ui_set_game_over(ui_state_t *ui, const char *winner, const char *score, const char *scores)
{
    ui->game_over = 1;
    snprintf(ui->game_result, sizeof(ui->game_result),
             "Vincitore: %s (%s celle) | Punteggi: %s", winner, score, scores);
    safe_copy(ui->status, sizeof(ui->status), "partita terminata");
}

void ui_add_event(ui_state_t *ui, const char *fmt, ...)
{
    va_list ap;
    int idx = ui->event_next;

    va_start(ap, fmt);
    vsnprintf(ui->events[idx], sizeof(ui->events[idx]), fmt, ap);
    va_end(ap);

    ui->event_next = (ui->event_next + 1) % UI_EVENTS_MAX;
    if (ui->event_count < UI_EVENTS_MAX)
    {
        ui->event_count++;
    }
}

void ui_render(const ui_state_t *ui, const char *input)
{
    if (!ui) return;

    /* Reset schermo e cursore in-place */
    fputs("\033[2J\033[3J\033[H", stdout);

    print_divider();
    puts("                                     CONQUISTA DEL TERRITORIO - CLIENT TCP                                     ");
    print_divider();

    if (ui->authenticated)
    {
        char my_col[32];
        get_player_color_code(ui->player_symbol, my_col, sizeof(my_col));
        printf("Utente: %s%s\033[0m (%s%s\033[0m)", my_col, ui->nickname, my_col, ui->player_symbol);
        if (ui->position_known)
        {
            printf(" | Posizione: (%d, %d)", ui->x, ui->y);
        }
        putchar('\n');
    }
    else
    {
        printf("Stato: \033[1;31mNON AUTENTICATO\033[0m (Usa 'register <nick> <pass>' o 'login <nick> <pass>')\n");
    }

    print_thin_divider();
    if (ui->event_count > 0)
    {
        int last_idx = (ui->event_next + UI_EVENTS_MAX - 1) % UI_EVENTS_MAX;
        printf("Ultimo evento: \033[1;33m%s\033[0m\n", ui->events[last_idx]);
    }
    else
    {
        printf("Ultimo evento: \033[1;33mIn attesa di connessione...\033[0m\n");
    }
    print_thin_divider();

    if (ui->game_over)
    {
        printf("\033[1;31mFINE PARTITA:\033[0m %s\n", ui->game_result);
        print_thin_divider();
    }

    printf(" MAPPA LOCALE (11x11)              │ MAPPA GLOBALE (30x15)\n");
    print_thin_divider();

    char local_rows[MAX_MAP_ROWS][MAX_ROW_LEN] = {{0}};
    char global_rows[MAX_MAP_ROWS][MAX_ROW_LEN] = {{0}};

    int num_local = (ui->authenticated && ui->local_valid) ? parse_map_rows(ui->local_map, local_rows) : 0;
    int num_global = (ui->authenticated && ui->global_valid) ? parse_map_rows(ui->global_map, global_rows) : 0;

    player_pos_t active_players[MAX_PLAYERS];
    int num_players = parse_player_positions(ui->positions, active_players, MAX_PLAYERS);

    int max_rows = num_local > num_global ? num_local : num_global;
    if (max_rows == 0)
    {
        puts("  In attesa di dati delle mappe...");
    }
    else
    {
        for (int r = 0; r < max_rows; ++r)
        {
            printf(" ");
            print_local_map_row(r < num_local ? local_rows[r] : NULL, ui->player_symbol);
            printf(" │ ");
            print_global_map_row(r < num_global ? global_rows[r] : NULL, r, active_players, num_players);
        }
    }

    if (ui->authenticated && ui->global_valid && ui->positions[0] != '\0' && strcmp(ui->positions, "-") != 0)
    {
        print_thin_divider();
        printf("Giocatori attivi: %s\n", ui->positions);
    }

    print_divider();
    puts("Comandi: w/a/s/d (movimento), users, quit");
    printf("> %s", input != NULL ? input : "");
    fflush(stdout);
}

void ui_finish(void)
{
    if (isatty(STDOUT_FILENO))
    {
        fputs("\033[0m\n", stdout);
        fflush(stdout);
    }
}