#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

// Plik do zapisu stanu magazynu
#define MAGAZYN_PLIK "magazyn_stan.dat"
#define PLIK_RAPORTU "raport.txt"

// Pojemnosc magazynu
#define MAGAZYN_POJEMNOSC 50

// Rozmiary skladnikow 
#define ROZMIAR_A 1
#define ROZMIAR_B 1
#define ROZMIAR_C 2
#define ROZMIAR_D 3

// Klucze IPC
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define KLUCZ_MSG 0x9999

// Indeksy semaforow
#define SEM_MUTEX 0       // Mutex do ochrony magazynu
#define SEM_WOLNE 1       // Liczba wolnych jednostek
#define SEM_SKLAD_A 2           // Dostepnosc skladnika A
#define SEM_SKLAD_B 3           // Dostepnosc skladnika B
#define SEM_SKLAD_C 4           // Dostepnosc skladnika C
#define SEM_SKLAD_D 5           // Dostepnosc skladnika D
#define SEM_COUNT 6       // Laczna liczba semaforow

typedef struct {
    long mtype;       // Typ komunikatu (musi byc > 0)
    char tekst[256];  // Tresc wiadomosci
} Komunikat;


// Struktura magazynu 
typedef struct {
    int skladnik_A;
    int skladnik_B;
    int skladnik_C;
    int skladnik_D;
    int wolne_miejsce;
} Magazyn;

#endif