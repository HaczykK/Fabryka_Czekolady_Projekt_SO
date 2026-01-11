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
    
    pid_t pid_dostawca_a = fork();
    if (pid_dostawca_a == 0) {
        execl("./bin/dostawca", "dostawca", "A", NULL);
        perror("execl dostawca A");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca A uruchomiony (PID: %d)\n", pid_dostawca_a);
    
    pid_t pid_dostawca_b = fork();
    if (pid_dostawca_b == 0) {
        execl("./bin/dostawca", "dostawca", "B", NULL);
        perror("execl dostawca B");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca B uruchomiony (PID: %d)\n", pid_dostawca_b);
    
    pid_t pid_dostawca_c = fork();
    if (pid_dostawca_c == 0) {
        execl("./bin/dostawca", "dostawca", "C", NULL);
        perror("execl dostawca C");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca C uruchomiony (PID: %d)\n", pid_dostawca_c);
    
    pid_t pid_dostawca_d = fork();
    if (pid_dostawca_d == 0) {
        execl("./bin/dostawca", "dostawca", "D", NULL);
        perror("execl dostawca D");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca D uruchomiony (PID: %d)\n", pid_dostawca_d);
    

    //uruchamianie pracownika
    printf("\n[DYREKTOR] Uruchamiam pracownika na stanowisku 1...\n");
    
     pid_t pid_pracownik_1 = fork();
    if (pid_pracownik_1 == 0) {
        execl("./bin/pracownik", "pracownik", "1", NULL);
        perror("execl pracownik 1");
        exit(1);
    }
    printf("[DYREKTOR] Pracownik 1 (TYP_1: A+B+C) uruchomiony (PID: %d)\n", pid_pracownik_1);
    
    pid_t pid_pracownik_2 = fork();
    if (pid_pracownik_2 == 0) {
        execl("./bin/pracownik", "pracownik", "2", NULL);
        perror("execl pracownik 2");
        exit(1);
    }
    printf("[DYREKTOR] Pracownik 2 (TYP_2: A+B+D) uruchomiony (PID: %d)\n", pid_pracownik_2);
    

    printf("[DYREKTOR] Oczekiwanie na zakonczenie procesow...\n\n");
    
    int status;
    pid_t pid;
    int zakonczone = 0;
    int liczba_procesow = 6;
    
    while (zakonczone < liczba_procesow) {
        pid = wait(&status);
        if (pid > 0) {
            printf("\n[DYREKTOR] Proces PID:%d zakonczyl prace (status: %d)\n", pid, WEXITSTATUS(status));
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