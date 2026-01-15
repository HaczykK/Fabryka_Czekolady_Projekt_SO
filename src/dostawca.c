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


int czy_bezpiecznie(int sem_id, int wolne, char typ) {
    // 1 opcja: malo miejsca (<15) - Blokujemy skladniki ktore zajmujace 2 i 3 bajty (C i D)
    if (wolne < 15) {
        int cnt_c = sem_getval(sem_id, SEM_SKLAD_C);
        int cnt_d = sem_getval(sem_id, SEM_SKLAD_D);
        
        // Jesli jest juz jakies C lub D, to nie dokladaj kolejnych
        if ((typ == 'C' && cnt_c > 0) || (typ == 'D' && cnt_d > 0)) return 0;
    }

    // 2 opcja: bardzo malo miejsca (<5) - tylko braki
    if (wolne < 5) {
        int val = 0;
        if (typ == 'A') val = sem_getval(sem_id, SEM_SKLAD_A);
        if (typ == 'B') val = sem_getval(sem_id, SEM_SKLAD_B);
        if (typ == 'C') val = sem_getval(sem_id, SEM_SKLAD_C);
        if (typ == 'D') val = sem_getval(sem_id, SEM_SKLAD_D);
        
        // Wpuszczamy tylko jesli tego skladnika calkowicie brakuje
        if (val == 0) return 1;
        return 0;
    }

    // 3 opcja limit nadprodukcji (zeby nie zapchac magazynu samym A)
    int limit = 20;
    int val = 0;
    if (typ == 'A') val = sem_getval(sem_id, SEM_SKLAD_A);
    if (typ == 'B') val = sem_getval(sem_id, SEM_SKLAD_B);
    if (typ == 'C') val = sem_getval(sem_id, SEM_SKLAD_C);
    if (typ == 'D') val = sem_getval(sem_id, SEM_SKLAD_D);
    
    if (val > limit) return 0;

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
    Magazyn* mag = polacz_z_pamiecia_dzielona(shm_id);
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

        // Sprawdzamy bezpieczenstwo (ile zajete = pojemnosc - wolne)
        int wolne_fizycznie = MAGAZYN_POJEMNOSC - mag->zajete;
        
        if (!czy_bezpiecznie(sem_id, wolne_fizycznie, skladnik)) {
            // Wycofujemy sie
            sem_signal(sem_id, SEM_MUTEX);
            for(int k=0; k<potrzebne_miejsce; k++) sem_signal(sem_id, SEM_WOLNE);
            usleep(200000); 
            continue;
        }

        for (int k=0; k<ilosc; k++) {
            wstaw_do_bufora(mag, skladnik, rozmiar);
        }
        

        sprintf(log_buf, "[DOSTAWCA-%c] Dostarczono %d x %c | Magazyn zajety: %d/%d |", 
                skladnik, ilosc, skladnik, mag->zajete, MAGAZYN_POJEMNOSC);
        wyslij_log(msg_id, log_buf);       
        

        sem_signal(sem_id, SEM_MUTEX);
        
        for (int j = 0; j < ilosc; j++) sem_signal(sem_id, sem_skladnik);
        
        sleep((rand() % 3) + 1);
    }
    
    sprintf(log_buf, "[DOSTAWCA-%c] Koniec pracy |\n", skladnik);
    wyslij_log(msg_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}