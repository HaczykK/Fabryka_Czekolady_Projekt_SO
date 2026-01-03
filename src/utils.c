#include <stdio.h>
#include "common.h"
#include "utils.h"


void wyswietl_stan_magazynu(Magazyn* mag) {
    printf("Stan magazynu");
    printf("Skladnik A: %d\n", mag->skladnik_A);
    printf("Skladnik B: %d\n", mag->skladnik_B);
    printf("Skladnik C: %d\n", mag->skladnik_C);
    printf("Skladnik D: %d\n", mag->skladnik_D);
    printf("Wolne miejsce: %d/%d\n", mag->wolne_miejsce, MAGAZYN_POJEMNOSC);
}