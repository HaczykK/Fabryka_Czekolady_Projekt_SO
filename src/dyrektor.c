#include <stdio.h>
#include "common.h"
#include "utils.h"


int main() {
    printf("Dyrektor: Start testu\n");

    Magazyn testowy;
    testowy.skladnik_A = 10;
    testowy.skladnik_B = 5;
    testowy.skladnik_C = 2;
    testowy.skladnik_D = 1;
    testowy.wolne_miejsce = MAGAZYN_POJEMNOSC - (10 * ROZMIAR_A + 5 * ROZMIAR_B + 2 * ROZMIAR_C + 1 * ROZMIAR_D);

    printf("Pojemnosc: %d jednostek\n", MAGAZYN_POJEMNOSC);
    printf("Rozmiary: A:%d, B:%d, C:%d, D:%d\n", ROZMIAR_A, ROZMIAR_B, ROZMIAR_C, ROZMIAR_D);

    wyswietl_stan_magazynu(&testowy);


    return 0;
}