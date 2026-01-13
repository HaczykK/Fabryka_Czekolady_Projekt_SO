CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -Iinclude -D_GNU_SOURCE
LDFLAGS = 

# Lista programów do zbudowania (USUNIETO bin/magazyn)
TARGETS = bin/dyrektor bin/dostawca bin/pracownik

all: directories $(TARGETS)

directories:
	mkdir -p bin

bin/dyrektor: src/dyrektor.c src/utils.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/dostawca: src/dostawca.c src/utils.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/pracownik: src/pracownik.c src/utils.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# USUNIETO REGULE DLA bin/magazyn

clean:
	rm -rf bin
	rm -f magazyn.dat

run: all
	./bin/dyrektor

.PHONY: all clean run directories