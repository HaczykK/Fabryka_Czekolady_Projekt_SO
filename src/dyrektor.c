#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "common.h"  
#include "utils.h"


int main() {
    printf("Dyrektor: Start testu\n");

    //Utworzenie pamieci dzielonej
    int shm_id = utworz_pamiec_dzielona();

    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);

    //Stowrzenie magazynu 
    printf("\n[DYREKTOR] Inicjalizacja magazynu:\n");
    
    magazyn->skladnik_A = 0;
    magazyn->skladnik_B = 0;
    magazyn->skladnik_C = 0;
    magazyn->skladnik_D = 0;
    magazyn->wolne_miejsce = MAGAZYN_POJEMNOSC;

    printf("[DYREKTOR] Magazyn utworzony\n\n");
    wyswietl_stan_magazynu(magazyn);    //stan poczatkowy magazynu
    printf("\n");

    //Utworzenie semaforow
    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);


//  Test 
    printf("\n[DYREKTOR] Uruchamiam dostawce skladnika A...\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Proces potomny - dostawca
        execl("./bin/dostawca", "dostawca", "A", NULL);
        perror("execl");
        exit(1);
    } else if (pid > 0) {
        // Proces rodzica - dyrektor
        printf("[DYREKTOR] Dostawca A uruchomiony (PID: %d)\n", pid);
        
        // Czekaj na zakonczenie dostawcy
        int status;
        waitpid(pid, &status, 0);
        
        printf("\n[DYREKTOR] Dostawca zakonczyl prace\n\n");
        
        // Pokaz koncowy stan magazynu
        wyswietl_stan_magazynu(magazyn);
        
    } else {
        perror("fork");
        exit(1);
    }
    
//  Sprzatanie
    printf("\n[DYREKTOR] Sprzatanie...\n");
    odlacz_pamiec_dzielona(magazyn);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
    
    
    return 0;
}