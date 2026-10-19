# Immagine base recente con ambiente GNU/Linux compatibile POSIX.
FROM ubuntu:24.04

# Installa toolchain C minima e pulisce la cache apt per tenere l'immagine piu leggera.
RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential make && \
    rm -rf /var/lib/apt/lists/*

# Tutti i comandi successivi lavorano nella directory applicativa.
WORKDIR /app

# Copia sorgenti, Makefile e documentazione nel container.
COPY . .

# Ricompila da zero dentro l'immagine.
RUN make clean && make

# Default: avvio server sulla porta 4242, durata 300s, update ogni 5s.
CMD ["./bin/server", "4242", "300", "5"]
