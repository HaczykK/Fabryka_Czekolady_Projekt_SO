#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include "utils.h"

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}


int czy_bezpiecznie_dostarczyc(Magazyn* m, char typ, int rozmiar) {
    int wolne = m->wolne_miejsce - rozmiar;

    // Opcja 1 (mniej niż 15 miejsc) -> Blokujemy C i D 
    if (wolne < 15) {
        if (typ == 'C' || typ == 'D') {
            // Wpuszczamy C lub D WYJĄTKOWO tylko jak ich w ogóle nie ma (stan 0)
            if (typ == 'C' && m->skladnik_C == 0) return 1;
            if (typ == 'D' && m->skladnik_D == 0) return 1;
            return 0;
        }
    }

    // Opcja 2 (mniej niż 5 miejsc) -> Wpuszczamy tylko skladnik ktorego nam brakuje do produkcji
    if (wolne < 5) {
        // Wpuszczamy TYLKO ten składnik, którego brakuje do zera
        if (typ == 'A' && m->skladnik_A == 0) return 1;
        if (typ == 'B' && m->skladnik_B == 0) return 1;
        if (typ == 'C' && m->skladnik_C == 0) return 1;
        if (typ == 'D' && m->skladnik_D == 0) return 1;
        
        return 0;
    }

    // Opcja: Limit nadprodukcji (zeby nie zapchać jednym typem)
    int limit = 20;
    if (typ == 'A' && m->skladnik_A > limit) return 0;
    if (typ == 'B' && m->skladnik_B > limit) return 0;
    if (typ == 'C' && m->skladnik_C > limit) return 0;
    if (typ == 'D' && m->skladnik_D > limit) return 0;

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    // Rejestracja sygnalow
    signal(SIGUSR2, handle_signal); // Stop od Dyrektora
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


    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
    int msg_id = polacz_kolejke();

    char log_buf[256];

    sprintf(log_buf, "[DOSTAWCA-%c] PID:%d Start pracy (rozmiar jednostki: %d)\n", skladnik, getpid(), rozmiar);
    wyslij_log(msg_id, log_buf);
    
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

        if (!czy_bezpiecznie_dostarczyc(magazyn, skladnik, rozmiar)) {
            // Jesli jest niebezpiecznie (ryzyko zapchania):
            
            // Wychodzimy z magazynu
            sem_signal(sem_id, SEM_MUTEX);
            
            // Ooddajemy miejsce ktore zarezerwowalismy
            for(int k=0; k<potrzebne_miejsce; k++) {
                sem_signal(sem_id, SEM_WOLNE);
            }
            
            // Czekamy chwilę i próbujemy od nowa pętli
            usleep(200000); 
            continue; 
        }
        

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
        
        sprintf(log_buf, "[DOSTAWCA-%c] Dostarczono %d x %c | Magazyn: A=%d B=%d C=%d D=%d | Wolne:%d/%d\n",
               skladnik, ilosc, skladnik,
               magazyn->skladnik_A, magazyn->skladnik_B,
               magazyn->skladnik_C, magazyn->skladnik_D,
               magazyn->wolne_miejsce, MAGAZYN_POJEMNOSC);
        
        wyslij_log(msg_id, log_buf);

        sem_signal(sem_id, SEM_MUTEX);
        
        for (int j = 0; j < ilosc; j++) sem_signal(sem_id, sem_skladnik);
        
        sleep((rand() % 3) + 1);
    }
    
    sprintf(log_buf, "[DOSTAWCA-%c] Koniec pracy.\n", skladnik);
    wyslij_log(msg_id, log_buf);
    odlacz_pamiec_dzielona(magazyn);
    return 0;
}