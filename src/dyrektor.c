#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "common.h"  
#include "utils.h"


pid_t pids[6];  // 4 dostawcow + 2 pracownikow
int liczba_procesow = 0;


int shutdown_requested = 0;

void handle_sigint(int sig) {
    (void)sig;
    shutdown_requested = 1;
    
    printf("\n\n[DYREKTOR] Otrzymano sygnal SIGINT (Ctrl+C)\n");
    printf("[DYREKTOR] Wysylanie SIGTERM do wszystkich procesow...\n");
    
    // Wyslij SIGTERM do wszystkich procesow potomnych
    for (int i = 0; i < liczba_procesow; i++) {
        if (pids[i] > 0) {
            printf("[DYREKTOR] Wysylam SIGTERM do PID:%d\n", pids[i]);
            kill(pids[i], SIGTERM);
        }
    }
}


int main() {
    printf("Dyrektor: Start testu\n");

    signal(SIGINT, handle_sigint);

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

    printf("\n[DYREKTOR] Uruchamiam 4 dostawcow (A, B, C, D)...\n");
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/dostawca", "dostawca", "A", NULL);
        perror("execl dostawca A");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca A uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/dostawca", "dostawca", "B", NULL);
        perror("execl dostawca B");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca B uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/dostawca", "dostawca", "C", NULL);
        perror("execl dostawca C");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca C uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/dostawca", "dostawca", "D", NULL);
        perror("execl dostawca D");
        exit(1);
    }
    printf("[DYREKTOR] Dostawca D uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    // 5. Uruchom 2 pracownikow
    printf("\n[DYREKTOR] Uruchamiam 2 pracownikow (stanowisko 1 i 2)...\n");
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/pracownik", "pracownik", "1", NULL);
        perror("execl pracownik 1");
        exit(1);
    }
    printf("[DYREKTOR] Pracownik 1 (TYP_1: A+B+C) uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    pids[liczba_procesow] = fork();
    if (pids[liczba_procesow] == 0) {
        execl("./bin/pracownik", "pracownik", "2", NULL);
        perror("execl pracownik 2");
        exit(1);
    }
    printf("[DYREKTOR] Pracownik 2 (TYP_2: A+B+D) uruchomiony (PID: %d)\n", pids[liczba_procesow]);
    liczba_procesow++;
    
    // 6. Czekaj na zakonczenie wszystkich procesow
    printf("\n[DYREKTOR] Fabryka dziala!\n");
    printf("[DYREKTOR] Nacisnij Ctrl+C aby zakonczyc...\n\n");
    
    int status;
    pid_t pid;
    int zakonczone = 0;
    
    while (zakonczone < liczba_procesow) {
        pid = wait(&status);
        if (pid > 0) {
            printf("\n[DYREKTOR] Proces PID:%d zakonczyl prace\n", pid);
            zakonczone++;
        }
    }
    
    // 7. Pokaz koncowy stan magazynu
    printf("\n[DYREKTOR] Wszystkie procesy zakonczone!\n\n");
    printf("=== KONCOWY STAN MAGAZYNU ===\n");
    wyswietl_stan_magazynu(magazyn);

//  Sprzatanie
    printf("\n[DYREKTOR] Sprzatanie...\n");
    odlacz_pamiec_dzielona(magazyn);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
    
    
    return 0;
}