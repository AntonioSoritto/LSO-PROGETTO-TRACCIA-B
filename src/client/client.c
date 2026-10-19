#include "client/client.h"

#include "client/ui.h"
#include "common/net.h"
#include "common/protocol.h"
#include "common/utils.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

/*
 * Contesto runtime del client.
 *
 * ui contiene lo stato gia interpretato da mostrare a schermo; input contiene
 * la riga che l'utente sta digitando. interactive distingue terminale reale
 * da pipe/file, perche nel primo caso si usano echo manuale e refresh ANSI.
 */
typedef struct {
    ui_state_t ui;
    char input[UI_INPUT_MAX];
    size_t input_len;
    int interactive;
} client_ctx_t;

static struct termios saved_termios;
static int termios_saved = 0;

static void restore_terminal(void) {
    // Ripristina il terminale se il client lo aveva messo in modalita non canonica.
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        termios_saved = 0;
    }
}

static void enable_terminal_mode(void) {
    struct termios raw;

    // In modalita interattiva leggiamo un byte alla volta, cosi w/a/s/d sono immediati.
    if (!isatty(STDIN_FILENO)) {
        return;
    }
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return;
    }
    raw = saved_termios;
    // ICANON disattivato: read restituisce subito i singoli tasti; ECHO disattivato: stampa la UI.
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    // VMIN/VTIME a zero rendono read non bloccante a livello di disciplina terminale.
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
        termios_saved = 1;
    }
}

static int send_command(int fd, const char *fmt, ...) {
    // Tutti i comandi applicativi sono righe terminate da '\n'.
    char payload[PROTO_MAX_LINE];
    char line[PROTO_MAX_LINE];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(payload, sizeof(payload), fmt, ap);
    va_end(ap);
    // Se il comando non entra nel buffer lo rifiutiamo: evita troncamenti silenziosi.
    if (n < 0 || (size_t)n >= sizeof(payload)) {
        return -1;
    }
    if (proto_make_line(line, sizeof(line), "%s", payload) < 0) {
        return -1;
    }
    return net_send_line(fd, line);
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    // Rimuove CR/LF finali per gestire sia UNIX sia input con terminatori CRLF.
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static int parse_two_numbers(char **tok, long *a, long *b) {
    int ok_a;
    int ok_b;

    // Usato per dimensioni mappa: il range e volutamente ampio ma finito.
    *a = utils_parse_long(tok[1], 1, 200, &ok_a);
    *b = utils_parse_long(tok[2], 1, 200, &ok_b);
    return ok_a && ok_b;
}

static void handle_server_line(client_ctx_t *ctx, const char *line) {
    // Traduce una riga del protocollo nello stato grafico mantenuto dalla UI.
    char copy[PROTO_MAX_LINE];
    char *tok[PROTO_MAX_TOKENS];
    int ntok;
    long w;
    long h;
    long x;
    long y;

    snprintf(copy, sizeof(copy), "%s", line);
    // proto_split modifica la stringa, quindi lavora su una copia della riga ricevuta.
    ntok = proto_split(copy, tok, PROTO_MAX_TOKENS);
    if (ntok == 0) {
        return;
    }

    if (strcmp(tok[0], "S2C_LOCAL_MAP") == 0 && ntok == 4) {
        // Formato: S2C_LOCAL_MAP <w> <h> <mappa_codificata>.
        if (parse_two_numbers(tok, &w, &h)) {
            ui_set_local_map(&ctx->ui, (int)w, (int)h, tok[3]);
            ui_add_event(&ctx->ui, "Mappa locale aggiornata");
        }
    } else if (strcmp(tok[0], "S2C_GLOBAL_UPDATE") == 0 && ntok == 5) {
        // Formato: S2C_GLOBAL_UPDATE <w> <h> <mappa_globale> <posizioni>.
        if (parse_two_numbers(tok, &w, &h)) {
            ui_set_global_map(&ctx->ui, (int)w, (int)h, tok[3], tok[4]);
            ui_add_event(&ctx->ui, "Aggiornamento globale ricevuto");
        }
    } else if (strcmp(tok[0], "S2C_USERS") == 0 && ntok == 2) {
        ui_set_positions(&ctx->ui, tok[1]);
        ui_add_event(&ctx->ui, "Utenti collegati: %s", tok[1]);
    } else if (strcmp(tok[0], "S2C_GAME_OVER") == 0 && ntok == 4) {
        ui_set_game_over(&ctx->ui, tok[1], tok[2], tok[3]);
        ui_add_event(&ctx->ui, "Partita terminata");
    } else if (strcmp(tok[0], "S2C_OK") == 0 && ntok >= 2) {
        if (strcmp(tok[1], "CONNECTED") == 0) {
            ui_set_connected(&ctx->ui, 1);
            ui_add_event(&ctx->ui, "Connessione stabilita");
        } else if (strcmp(tok[1], "REGISTERED") == 0) {
            ui_add_event(&ctx->ui, "Registrazione completata");
        } else if (strcmp(tok[1], "LOGGED_IN") == 0 && ntok == 6) {
            int ok_x;
            int ok_y;
            // Il server ritorna nickname, simbolo giocatore e coordinate iniziali.
            x = utils_parse_long(tok[4], 0, 10000, &ok_x);
            y = utils_parse_long(tok[5], 0, 10000, &ok_y);
            ui_set_user(&ctx->ui, tok[2], tok[3]);
            if (ok_x && ok_y) {
                ui_set_position(&ctx->ui, (int)x, (int)y);
            }
            ui_add_event(&ctx->ui, "Login effettuato");
        } else if (strcmp(tok[1], "MOVED") == 0 && ntok == 4) {
            int ok_x;
            int ok_y;
            x = utils_parse_long(tok[2], 0, 10000, &ok_x);
            y = utils_parse_long(tok[3], 0, 10000, &ok_y);
            if (ok_x && ok_y) {
                ui_set_position(&ctx->ui, (int)x, (int)y);
                ui_add_event(&ctx->ui, "Movimento eseguito: posizione (%ld,%ld)", x, y);
            }
        } else if (strcmp(tok[1], "BYE") == 0) {
            ui_add_event(&ctx->ui, "Disconnessione confermata dal server");
        } else {
            ui_add_event(&ctx->ui, "OK dal server");
        }
    } else if (strcmp(tok[0], "S2C_ERR") == 0 && ntok >= 2) {
        ui_add_event(&ctx->ui, "Errore server: %s", tok[1]);
    } else {
        ui_add_event(&ctx->ui, "Messaggio server non riconosciuto");
    }
}

static int process_socket(int fd, client_ctx_t *ctx, char *buf, size_t *len) {
    // TCP e uno stream: accumuliamo byte finche non troviamo righe complete.
    char tmp[512];
    ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

    if (n < 0 && errno == EINTR) {
        return 0;
    }
    if (n <= 0) {
        ui_set_connected(&ctx->ui, 0);
        ui_add_event(&ctx->ui, "Connessione chiusa dal server");
        return -1;
    }
    if (*len + (size_t)n >= PROTO_MAX_LINE) {
        ui_add_event(&ctx->ui, "Messaggio server troppo lungo");
        return -1;
    }
    memcpy(buf + *len, tmp, (size_t)n);
    *len += (size_t)n;
    buf[*len] = '\0';

    while (1) {
        char *nl = memchr(buf, '\n', *len);
        char line[PROTO_MAX_LINE];
        size_t line_len;
        if (nl == NULL) {
            break;
        }
        // Estrae una sola riga completa e lascia nel buffer eventuali byte successivi.
        line_len = (size_t)(nl - buf);
        memcpy(line, buf, line_len);
        line[line_len] = '\0';
        memmove(buf, nl + 1, *len - line_len - 1);
        *len -= line_len + 1;
        buf[*len] = '\0';
        handle_server_line(ctx, line);
    }
    return 0;
}

static int execute_command(int fd, client_ctx_t *ctx, char *line) {
    char copy[PROTO_MAX_LINE];
    char *tok[PROTO_MAX_TOKENS];
    int ntok;
    direction_t dir;

    trim_newline(line);
    snprintf(copy, sizeof(copy), "%s", line);
    ntok = proto_split(copy, tok, PROTO_MAX_TOKENS);
    if (ntok == 0) {
        return 0;
    }

    if (strcmp(tok[0], "register") == 0 && ntok == 3) {
        ui_add_event(&ctx->ui, "Invio registrazione per %s", tok[1]);
        return send_command(fd, "C2S_REGISTER %s %s", tok[1], tok[2]);
    }
    if (strcmp(tok[0], "login") == 0 && ntok == 3) {
        ui_add_event(&ctx->ui, "Invio login per %s", tok[1]);
        return send_command(fd, "C2S_LOGIN %s %s", tok[1], tok[2]);
    }
    if (strcmp(tok[0], "move") == 0 && ntok == 2 && proto_parse_direction(tok[1], &dir) == 0) {
        return send_command(fd, "C2S_MOVE %s", proto_direction_name(dir));
    }
    if (ntok == 1 && proto_parse_direction(tok[0], &dir) == 0) {
        return send_command(fd, "C2S_MOVE %s", proto_direction_name(dir));
    }
    if (strcmp(tok[0], "quit") == 0 && ntok == 1) {
        ui_add_event(&ctx->ui, "Uscita richiesta");
        if (send_command(fd, "C2S_QUIT") != 0) {
            return -1;
        }
        return 1;
    }

    ui_add_event(&ctx->ui, "Comando non riconosciuto: %s", tok[0]);
    return 0;
}

static int handle_input_byte(int fd, client_ctx_t *ctx, unsigned char ch) {
    char command[UI_INPUT_MAX];
    direction_t dir;

    // Ctrl+D e EOF vengono trattati come uscita pulita.
    if (ch == 4) {
        send_command(fd, "C2S_QUIT");
        return 1;
    }

    // Movimento immediato con WASD a prompt vuoto
    if (ctx->input_len == 0 && (ch == 'w' || ch == 'a' || ch == 's' || ch == 'd')) {
        command[0] = (char)ch;
        command[1] = '\0';
        if (proto_parse_direction(command, &dir) == 0) {
            // IL RETURN È FONDAMENTALE: invia il comando ed esce SENZA aggiungere la lettera a ctx->input
            return send_command(fd, "C2S_MOVE %s", proto_direction_name(dir));
        }
    }

    if (ch == '\r' || ch == '\n') {
        // Invio conferma il comando accumulato in ctx->input.
        snprintf(command, sizeof(command), "%s", ctx->input);
        ctx->input[0] = '\0';
        ctx->input_len = 0;
        return execute_command(fd, ctx, command);
    }

    if (ch == 127 || ch == 8) {
        // Gestione Backspace per terminali che inviano DEL (127) o BS (8).
        if (ctx->input_len > 0) {
            ctx->input[--ctx->input_len] = '\0';
        }
        return 0;
    }

    if (isprint(ch) && ctx->input_len + 1 < sizeof(ctx->input)) {
        // I caratteri stampabili vengono aggiunti al prompt locale.
        ctx->input[ctx->input_len++] = (char)ch;
        ctx->input[ctx->input_len] = '\0';
    }

    return 0;
}

static int process_stdin(int fd, client_ctx_t *ctx, int *command_done) {
    // Gestisce sia terminale interattivo sia input da pipe/file.
    unsigned char tmp[128];
    ssize_t n;
    ssize_t i;
    int rc = 0;

    *command_done = 0;
    n = read(STDIN_FILENO, tmp, sizeof(tmp));
    if (n == 0) {
        // EOF su stdin: chiusura pulita, utile per test con input rediretto.
        send_command(fd, "C2S_QUIT");
        return 1;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return 0;
        }
        return -1;
    }
    for (i = 0; i < n; ++i) {
        // In modalita raw possono arrivare piu byte insieme: si processano uno per volta.
        rc = handle_input_byte(fd, ctx, tmp[i]);
        if (tmp[i] == '\r' || tmp[i] == '\n') {
            *command_done = 1;
        }
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

int client_run(const char *host, const char *port) {
    int fd = net_connect_tcp(host, port);
    char sock_buf[PROTO_MAX_LINE];
    size_t sock_len = 0;
    int running = 1;
    int exit_status = 0;
    client_ctx_t ctx;

    if (fd < 0) {
        return -1;
    }

    // Evita terminazione del processo se il server chiude il socket durante una send.
    signal(SIGPIPE, SIG_IGN);
    memset(&ctx, 0, sizeof(ctx));
    ctx.interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    ui_init(&ctx.ui, host, port);
    ui_set_connected(&ctx.ui, 1);
    ui_add_event(&ctx.ui, "Connesso a %s:%s", host, port);

    enable_terminal_mode();
    ui_render(&ctx.ui, ctx.input);

    while (running) {
        fd_set rfds;
        int maxfd = fd > STDIN_FILENO ? fd : STDIN_FILENO;
        int rc;
        int should_render = 0;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        // select permette di ricevere update dal server mentre l'utente digita.
        rc = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            exit_status = -1;
            break;
        }
        if (FD_ISSET(fd, &rfds)) {
            // Priorita agli eventi di rete: possono aggiornare mappe, errori o game over.
            if (process_socket(fd, &ctx, sock_buf, &sock_len) != 0) {
                running = 0;
            }
            should_render = 1;
        }
        if (running && FD_ISSET(STDIN_FILENO, &rfds)) {
            int command_done = 0;
            rc = process_stdin(fd, &ctx, &command_done);
            if (rc < 0) {
                exit_status = -1;
                running = 0;
            } else if (rc > 0) {
                running = 0;
            }
            // Aggiorna sempre lo schermo ad ogni carattere digitato per dare immediato feedback visivo
            should_render = 1;
        }

        if (should_render) {
            ui_render(&ctx.ui, ctx.input);
        }
    }

    ui_render(&ctx.ui, ctx.input);
    restore_terminal();
    ui_finish();
    close(fd);
    return exit_status;
}