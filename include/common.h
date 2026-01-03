#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

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

// Struktura magazynu 
typedef struct {
    int skladnik_A;
    int skladnik_B;
    int skladnik_C;
    int skladnik_D;
    int wolne_miejsce;
} Magazyn;

#endif