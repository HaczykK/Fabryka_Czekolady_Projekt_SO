#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "common.h"
#include "utils.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}


int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    // Rejestracja sygnalow
    signal(SIGUSR1, handle_signal); // Stop od Dyrektora
    signal(SIGTERM, handle_signal);

    char skladnik = argv[1][0];
    int rozmiar; 
    int sem_skladnik;

    switch(skladnik) {
    case 'A':
        rozmiar = ROZMIAR_A;
        sem_skladnik = SEM_SKLAD_A;
        break;
    case 'B':
        rozmiar = ROZMIAR_B;
        sem_skladnik = SEM_SKLAD_B;
        break;
    case 'C':
        rozmiar = ROZMIAR_C;
        sem_skladnik = SEM_SKLAD_C;
        break;
    case 'D':
        rozmiar = ROZMIAR_D;
        sem_skladnik = SEM_SKLAD_D;
        break;
    default:
        fprintf(stderr, "Nieprawidlowy skladnik: %c\n", skladnik);
        return 1;
    }

    printf("[DOSTAWCA-%c] PID:%d Start pracy (rozmiar jednostki: %d)\n", skladnik, getpid(), rozmiar);

    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
    
    srand(time(NULL) + getpid());

    while (running) {
        int ilosc = (rand() % 2) + 1; // Male porcje (1-2)
        int potrzebne_miejsce = ilosc * rozmiar;
        
        for (int j = 0; j < potrzebne_miejsce; j++) {
            sem_wait(sem_id, SEM_WOLNE);
            if (!running) break;
        }
        if (!running) break;

        sem_wait(sem_id, SEM_MUTEX);
        

        switch(skladnik) {
            case 'A': 
                magazyn->skladnik_A += ilosc;
                break;
            case 'B': 
                magazyn->skladnik_B += ilosc; 
                break;
            case 'C': 
                magazyn->skladnik_C += ilosc; 
                break;
            case 'D': 
                magazyn->skladnik_D += ilosc; 
                break;
        }
        
        magazyn->wolne_miejsce -= potrzebne_miejsce;
        
        printf("[DOSTAWCA-%c] Dostarczono %d x %c | Magazyn: A=%d B=%d C=%d D=%d | Wolne:%d/%d\n",
               skladnik, ilosc, skladnik,
               magazyn->skladnik_A, magazyn->skladnik_B,
               magazyn->skladnik_C, magazyn->skladnik_D,
               magazyn->wolne_miejsce, MAGAZYN_POJEMNOSC);
        
        sem_signal(sem_id, SEM_MUTEX);
        
        for (int j = 0; j < ilosc; j++) sem_signal(sem_id, sem_skladnik);
        
        sleep((rand() % 3) + 1);
    }
    
    printf("[DOSTAWCA-%c] Koniec pracy.\n", skladnik);
    odlacz_pamiec_dzielona(magazyn);
    return 0;
}