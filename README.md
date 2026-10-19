# Territory Conquest TCP

Progetto didattico in C/POSIX per UNIX/Linux: sistema client-server multiutente via TCP per un gioco di conquista territorio su griglia.

Il server mantiene la mappa completa, lo stato dei giocatori e il timeout globale della partita. I client vedono pubblicamente la proprieta delle celle, ma scoprono gli ostacoli solo quando entrano nella zona esplorata dal proprio giocatore.

## Compilazione nativa

Richiede Linux/Ubuntu, `gcc` e `make`.

```sh
make
```

Gli eseguibili vengono creati in `bin/`:

```sh
bin/server
bin/client
```

## Esecuzione nativa

Avvio server:

```sh
./bin/server 4242 300 5
```

Argomenti server:

- `4242`: porta TCP.
- `300`: durata partita in secondi.
- `5`: periodo aggiornamenti globali in secondi.

Avvio client:

```sh
./bin/client 127.0.0.1 4242
```

`127.0.0.1` va usato solo se il client gira sulla stessa macchina del server. Se il server e remoto, bisogna usare l'IP o il DNS del server, per esempio:

```sh
./bin/client 203.0.113.10 4242
```

## Comandi client

Dopo la connessione:

```txt
register <nickname> <password>
login <nickname> <password>
move up|down|left|right
users
local
global
help
quit
```

Comandi brevi equivalenti:

```txt
w a s d
l h q
```

Il client riceve anche aggiornamenti periodici dal server con mappa globale e posizioni correnti.

## Esecuzione Docker

Build immagine:

```sh
docker build -t territory-conquest .
```

Server:

```sh
docker network create tc-net
docker run --rm --name tc-server --network tc-net -p 4242:4242 territory-conquest ./bin/server 4242 300 5
```

Client:

```sh
docker run --rm -it --network tc-net territory-conquest ./bin/client tc-server 4242
```

Se il client non e nello stesso network Docker del server, anche qui non va usato `127.0.0.1`: serve il nome DNS del container o l'IP/DNS della macchina remota.

Con Docker Compose:

```sh
docker compose up --build server
docker compose run --rm client
```

Per aprire piu client:

```sh
docker compose run --rm client
docker compose run --rm client
```

## Note progettuali

- Server single-process basato esplicitamente su `select(2)`.
- Protocollo applicativo testuale con framing a righe terminate da `\n`.
- Il server sceglie casualmente una mappa fra layout statici compilati nel codice.
- Utenti, sessioni client e giocatori sono gestiti con array dinamici riallocabili. Usando `select(2)`, il numero di socket resta vincolato da `FD_SETSIZE` e dai limiti del sistema operativo.
- Spawn casuale dei giocatori su celle libere e non occupate.
- La mappa locale e una finestra 11x11 centrata sul giocatore; la mappa globale mostra tutta la griglia di proprieta.
- Gli slot giocatore restano associati al nickname durante la partita, cosi le celle conquistate non cambiano proprietario se un client si disconnette e un altro entra.
- Ogni giocatore riceve un identificatore testuale stabile (`P0`, `P1`, ...), quindi non c'e collisione dopo le prime lettere/cifre.
- Nessun database esterno: utenti e stato partita sono mantenuti in memoria.
- Nessuna libreria esterna oltre a libc/POSIX.
- Il server non legge da `stdin`.
