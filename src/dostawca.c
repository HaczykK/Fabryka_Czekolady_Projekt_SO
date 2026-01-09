#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uzycie: %s <skladnik>\n", argv[0]);
        return 1;
    }
    
    char skladnik = argv[1][0];
    int rozmiar;
    int sem_skladnik;

    //przypisanie odpowiedniego skladnika
    switch(skladnik) {
        case 'A':
            rozmiar = ROZMIAR_A;
            sem_skladnik = SEM_A;
            break;
        case 'B':
            rozmiar = ROZMIAR_B;
            sem_skladnik = SEM_B;
            break;
        case 'C':
            rozmiar = ROZMIAR_C;
            sem_skladnik = SEM_C;
            break;
        case 'D':
            rozmiar = ROZMIAR_D;
            sem_skladnik = SEM_D;
            break;
        default:
            fprintf(stderr, "Nieprawidlowy skladnik: %c", skladnik);
            return 1;
    }

    //informacja o dostawcy
    printf("[DOSTAWCA-%c] PID:%d Start pracy (rozmiar jednostki: %d)\n", skladnik, getpid(), rozmiar);

    // Polacz sie z pamiecia dzielona i semaforami
    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
    
    printf("[DOSTAWCA-%c] Polaczono z magazynem (SHM:%d, SEM:%d)\n", skladnik, shm_id, sem_id);

    // Inicjalizacja losowania
    srand(time(NULL) + getpid());

    // Petla dostarczania skladnikow
    for (int i = 0; i < 10; i++) {  // 10 dostaw na test
        // Losowa ilosc (1-3 sztuki)
        int ilosc = (rand() % 3) + 1;
        int potrzebne_miejsce = ilosc * rozmiar;
        
        printf("[DOSTAWCA-%c] Probuje dostarczyc %d x %c (%d jednostek)...\n", skladnik, ilosc, skladnik, potrzebne_miejsce);
        
        // Petla sprawdzajaca czy w magazynie jest miejsce
        for (int j = 0; j < potrzebne_miejsce; j++) {
            sem_wait(sem_id, SEM_WOLNE);
        }
        
        sem_wait(sem_id, SEM_MUTEX);
        
        // Dodaje skladniku do magazynu
        switch(skladnik) {
            case 'A': magazyn->skladnik_A += ilosc; break;
            case 'B': magazyn->skladnik_B += ilosc; break;
            case 'C': magazyn->skladnik_C += ilosc; break;
            case 'D': magazyn->skladnik_D += ilosc; break;
        }
        magazyn->wolne_miejsce -= potrzebne_miejsce;
        
        printf("[DOSTAWCA-%c] Dostarczono %d x %c | Magazyn: A=%d B=%d C=%d D=%d | Wolne:%d/%d\n",
               skladnik, ilosc, skladnik,
               magazyn->skladnik_A, magazyn->skladnik_B,
               magazyn->skladnik_C, magazyn->skladnik_D,
               magazyn->wolne_miejsce, MAGAZYN_POJEMNOSC);
        
        // Sygnalizuj dostepnosc skladnika
        for (int j = 0; j < ilosc; j++) {
            sem_signal(sem_id, sem_skladnik);
        }
        
        // Wyjdz z sekcji krytycznej
        sem_signal(sem_id, SEM_MUTEX);
        
        // Czekaj losowy czas (1-3 sekundy)
        sleep((rand() % 3) + 1);
    }
    
    printf("[DOSTAWCA-%c] Koniec pracy\n", skladnik);
    
    // Odlacz sie od pamieci
    odlacz_pamiec_dzielona(magazyn);
    
    return 0;
}