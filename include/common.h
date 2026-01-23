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

// Indeksy semaforow
#define SEM_MUTEX 0       // Mutex do ochrony pamieci
#define SEM_WOLNE 1       // Liczba wolnych bajtow w magazynie

// Dostepnosc towarow 
#define SEM_SKLAD_A 2     
#define SEM_SKLAD_B 3     
#define SEM_SKLAD_C 4     
#define SEM_SKLAD_D 5

// Limit miejsca w kolejkach 
#define SEM_LIMIT_A 6
#define SEM_LIMIT_B 7
#define SEM_LIMIT_C 8
#define SEM_LIMIT_D 9

// Logowanie
#define SEM_LOG 10

// Laczna liczba semaforow
#define SEM_COUNT 11       

// Pojemnosc kolejek dla kazdego skladnika - 7 * 7 bajtow = 49 < 50
#define KOLEJKA_POJEMNOSC 7

// Ring buffer FIFO dla pojedynczego typu skladnika
typedef struct {
    char dane[KOLEJKA_POJEMNOSC]; // Fizyczna tablica na dane
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