#include <stdio.h>
#include <unistd.h>
#include "common.h"
#include "utils.h"


int main() {
    printf("Dyrektor: Start testu\n");

    int shm_id = utworz_pamiec_dzielona();

    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);


    printf("\n[DYREKTOR] Inicjalizacja magazynu:\n");
    
    magazyn->skladnik_A = 0;
    magazyn->skladnik_B = 0;
    magazyn->skladnik_C = 0;
    magazyn->skladnik_D = 0;
    magazyn->wolne_miejsce = MAGAZYN_POJEMNOSC;

    printf("[DYREKTOR] Magazyn utworzony\n\n");
    wyswietl_stan_magazynu(magazyn);

// 4. Test - dodaj troche skladnikow
    printf("\n[DYREKTOR] Test: Dodaje skladniki...\n");
    magazyn->skladnik_A = 5;
    magazyn->skladnik_B = 3;
    magazyn->wolne_miejsce -= (5 * ROZMIAR_A + 3 * ROZMIAR_B);
    
    printf("\n");
    wyswietl_stan_magazynu(magazyn);
    
    // 5. Poczekaj chwile (zeby bylo widac)
    printf("\n[DYREKTOR] Czekam 2 sekundy...\n");
    sleep(2);
    
    // 6. Sprzatanie
    printf("\n[DYREKTOR] Sprzatanie...\n");
    odlacz_pamiec_dzielona(magazyn);
    usun_pamiec_dzielona(shm_id);
    
    
    return 0;
}