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

    //uruchamianie dostawcow (A,B,C)
    printf("\n[DYREKTOR] Uruchamiam dostawcow...\n");
    
    pid_t pid_a = fork();
    if (pid_a == 0) {
        execl("./bin/dostawca", "dostawca", "A", NULL);
        perror("execl dostawca A");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca A uruchomiony (PID: %d)\n", pid_a);
    
    pid_t pid_b = fork();
    if (pid_b == 0) {
        execl("./bin/dostawca", "dostawca", "B", NULL);
        perror("execl dostawca B");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca B uruchomiony (PID: %d)\n", pid_b);
    
    pid_t pid_c = fork();
    if (pid_c == 0) {
        execl("./bin/dostawca", "dostawca", "C", NULL);
        perror("execl dostawca C");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca C uruchomiony (PID: %d)\n", pid_c);
    

    //uruchamianie pracownika
    printf("\n[DYREKTOR] Uruchamiam pracownika na stanowisku 1...\n");
    
    pid_t pid_prac1 = fork();
    if (pid_prac1 == 0) {
        execl("./bin/pracownik", "pracownik", "1", NULL);
        perror("execl pracownik");
        exit(1);
    }
    printf("[DYREKTOR] Pracownik 1 uruchomiony (PID: %d)\n\n", pid_prac1);
    

    printf("[DYREKTOR] Oczekiwanie na zakonczenie procesow...\n\n");
    
    int status;
    pid_t pid;
    int zakonczone = 0;
    
    while (zakonczone < 4) {
        pid = wait(&status);
        if (pid > 0) {
            printf("[DYREKTOR] Proces PID:%d zakonczyl prace\n", pid);
            zakonczone++;
        }
    }
    
    
    printf("\n[DYREKTOR] Wszystkie procesy zakonczone\n\n");
    wyswietl_stan_magazynu(magazyn);

//  Sprzatanie
    printf("\n[DYREKTOR] Sprzatanie...\n");
    odlacz_pamiec_dzielona(magazyn);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
    
    
    return 0;
}