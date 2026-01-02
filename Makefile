CC = gcc
CFLAGS = -Wall -I./include

all: bin/dyrektor bin/dostawca bin/pracownik

bin/dyrektor: src/dyrektor.c src/utils.c
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@

bin/dostawca: src/dostawca.c src/utils.c
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@

bin/pracownik: src/pracownik.c src/utils.c
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf bin obj

test: all
	./bin/dyrektor
	./bin/dostawca A
	./bin/pracownik 1

.PHONY: all clean test