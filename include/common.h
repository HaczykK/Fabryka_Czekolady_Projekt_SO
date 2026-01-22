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

// Bajty reprezentujace skladniki
#define BAJT_A 'A'
#define BAJT_B 'B'
#define BAJT_C 'C'
#define BAJT_D 'D'
#define BAJT_PUSTY '.'

// Kolory dla terminala
#define KOLOR_RESET   "\033[0m"
#define KOLOR_CZERWONY "\033[31m"
#define KOLOR_ZIELONY  "\033[32m"
#define KOLOR_ZOLTY    "\033[33m"
#define KOLOR_NIEBIESKI "\033[34m"
#define KOLOR_MAGENTA  "\033[35m"
#define KOLOR_CYAN     "\033[36m"
#define KOLOR_BIALY    "\033[37m"
#define KOLOR_BOLD     "\033[1m"

// Klucze IPC
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define KLUCZ_MSG 0x9999

// Indeksy semaforow
#define SEM_MUTEX 0       // Mutex do ochrony magazynu
#define SEM_WOLNE 1       // Liczba wolnych jednostek
#define SEM_SKLAD_A 2     // Dostepnosc skladnika A
#define SEM_SKLAD_B 3     // Dostepnosc skladnika B
#define SEM_SKLAD_C 4     // Dostepnosc skladnika C
#define SEM_SKLAD_D 5     // Dostepnosc skladnika D
#define SEM_COUNT 6       // Laczna liczba semaforow

typedef struct {
    long mtype;       // Typ komunikatu (musi byc > 0)
    char tekst[512];  // Tresc wiadomosci
} Komunikat;


// Pojemnosc kolejek per skladnik (~25% kazdej)
#define KOLEJKA_POJEMNOSC 12

// Ring buffer FIFO dla pojedynczego typu skladnika
typedef struct {
    char dane[KOLEJKA_POJEMNOSC]; // <--- ZMIANA: Fizyczna tablica na dane
    int head;       // Indeks do wstawiania
    int tail;       // Indeks do pobierania
    int count;      // Liczba elementow w kolejce
} RingQueue;

// Struktura magazynu - osobne kolejki FIFO dla kazdego skladnika
typedef struct {
    RingQueue kolejka_A;
    RingQueue kolejka_B;
    RingQueue kolejka_C;
    RingQueue kolejka_D;
    
    int suma_bajtow;  // Laczna liczba zajetych bajtow (dla limitu MAGAZYN_POJEMNOSC)
} Magazyn;

#endif