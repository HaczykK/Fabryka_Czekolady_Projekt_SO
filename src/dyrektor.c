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


    printf("\n");
    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);


//  Test 
    printf("\n[DYREKTOR] Test operacji P (wait) i V (signal)\n");
    
    printf("\n[TEST] Wartosc SEM_MUTEX przed: %d\n", sem_getval(sem_id, SEM_MUTEX));
    printf("[TEST] Wywoluje sem_wait(MUTEX)...\n");
    sem_wait(sem_id, SEM_MUTEX);
    printf("[TEST] Wartosc SEM_MUTEX po wait: %d (sekcja krytyczna)\n", sem_getval(sem_id, SEM_MUTEX));
    
    printf("[TEST] Symulacja pracy w sekcji krytycznej...\n");
    sleep(1);
    
    printf("[TEST] Wywoluje sem_signal(MUTEX)...\n");
    sem_signal(sem_id, SEM_MUTEX);
    printf("[TEST] Wartosc SEM_MUTEX po signal: %d (zwolniono)\n", sem_getval(sem_id, SEM_MUTEX));
    
//  Sprzatanie
    printf("\n[DYREKTOR] Sprzatanie...\n");
    odlacz_pamiec_dzielona(magazyn);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
    
    
    return 0;
}