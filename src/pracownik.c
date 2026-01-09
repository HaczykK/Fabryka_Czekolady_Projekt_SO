#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Brak argumentu!\n");
        return 1;
    }
    
     int stanowisko = atoi(argv[1]);
    
    if (stanowisko != 1 && stanowisko != 2) {
        fprintf(stderr, "Nieprawidlowe stanowisko: %d (dozwolone: 1 lub 2)\n", stanowisko);
        return 1;
    }
    
    const char* typ_czekolady = (stanowisko == 1) ? "TYP_1 (A+B+C)" : "TYP_2 (A+B+D)";
    
    printf("[PRACOWNIK-%d] PID:%d Start - produkuje czekolade %s\n", stanowisko, getpid(), typ_czekolady);

    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
    
    printf("[PRACOWNIK-%d] Polaczono z magazynem (SHM:%d, SEM:%d)\n", stanowisko, shm_id, sem_id);

    srand(time(NULL) + getpid());
    
    int wyprodukowano = 0;

    // Petla produkcyjna
    for (int i = 0; i < 5; i++) {  // 5 czekolad na test
        printf("[PRACOWNIK-%d] Czekam na skladniki...\n", stanowisko);
        
        // Czekanie na sygnal czy sa dostepne
        sem_wait(sem_id, SEM_A);  // Potrzebne A
        sem_wait(sem_id, SEM_B);  // Potrzebne B
        
        if (stanowisko == 1) {
            sem_wait(sem_id, SEM_C);  // Stanowisko 1 potrzebuje C
        } else {
            sem_wait(sem_id, SEM_D);  // Stanowisko 2 potrzebuje D
        }
        
        
        sem_wait(sem_id, SEM_MUTEX);
        
        //Zabranie skladnika z magazynu
        magazyn->skladnik_A -= 1;
        magazyn->skladnik_B -= 1;
        
        int zwolnione_miejsce = ROZMIAR_A + ROZMIAR_B;
        
        if (stanowisko == 1) {
            magazyn->skladnik_C -= 1;
            zwolnione_miejsce += ROZMIAR_C;
        } else {
            magazyn->skladnik_D -= 1;
            zwolnione_miejsce += ROZMIAR_D;
        }
        
        magazyn->wolne_miejsce += zwolnione_miejsce;

        printf("[PRACOWNIK-%d] Pobrano skladniki | Magazyn: A=%d B=%d C=%d D=%d | Wolne:%d/%d\n",
               stanowisko,
               magazyn->skladnik_A, magazyn->skladnik_B,
               magazyn->skladnik_C, magazyn->skladnik_D,
               magazyn->wolne_miejsce, MAGAZYN_POJEMNOSC);
        


        sem_signal(sem_id, SEM_MUTEX);
        
        // Jesli pobrano skladnik to jest zwalniane miejsce w magazynie
        for (int j = 0; j < zwolnione_miejsce; j++) {
            sem_signal(sem_id, SEM_WOLNE);
        }
        
        // Symulacja produkcji
        int czas_produkcji = (rand() % 2) + 1;  // 1-2 sekundy
        printf("[PRACOWNIK-%d] Produkuje czekolade %s... (%ds)\n", stanowisko, typ_czekolady, czas_produkcji);
        sleep(czas_produkcji);
        
        wyprodukowano++;
        printf("[PRACOWNIK-%d] *** WYPRODUKOWANO CZEKOLADE %s #%d ***\n", stanowisko, typ_czekolady, wyprodukowano);
    }

    printf("[PRACOWNIK-%d] Koniec pracy - wyprodukowano %d czekolad\n",  stanowisko, wyprodukowano);
    
    // Odlacz sie od pamieci
    odlacz_pamiec_dzielona(magazyn);
    
    return 0;
}