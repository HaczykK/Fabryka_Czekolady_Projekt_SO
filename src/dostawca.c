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


// Sprawdza czy mozna bezpiecznie wstawic skladnik (zapobiega deadlock)
int czy_bezpiecznie(Magazyn* mag, char typ) {
    int wolne = MAGAZYN_POJEMNOSC - mag->suma_bajtow;
    
    // Sprawdz limit kolejki dla tego skladnika
    if (!czy_mozna_wstawic(mag, typ)) return 0;
    
    // 1 opcja: malo miejsca (<15) - Blokujemy skladniki zajmujace 2 i 3 bajty (C i D)
    if (wolne < 10) {
        int cnt_c = mag->kolejka_C.count;
        int cnt_d = mag->kolejka_D.count;
        
        // Jesli jest juz jakies C lub D, to nie dokladaj kolejnych
        if ((typ == 'C' && cnt_c > 0) || (typ == 'D' && cnt_d > 0)) return 0;
    }

    // 2 opcja: bardzo malo miejsca (<5) - tylko braki
    if (wolne < 5) {
        int val = zlicz_skladnik(mag, typ);
        
        // Wpuszczamy tylko jesli tego skladnika calkowicie brakuje
        if (val == 0) return 1;
        return 0;
    }

    // 3 opcja limit nadprodukcji (zeby nie zapchac magazynu samym A)
    int limit = KOLEJKA_POJEMNOSC - 2; // Max 10 w kolejce
    int val = zlicz_skladnik(mag, typ);
    
    if (val >= limit) return 0;

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    // Rejestracja sygnalow
    signal(SIGUSR2, handle_signal);
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
    // msg_id USUNIETY

    char log_buf[256];

    sprintf(log_buf, "%s[DOSTAWCA-%c]%s PID:%d Start pracy (rozmiar jednostki: %d)", 
            KOLOR_ZIELONY, skladnik, KOLOR_RESET, getpid(), rozmiar);
    wyslij_log(sem_id, log_buf);
    
    srand(time(NULL) + getpid());

    int czy_czekam = 0;

    while (running) {
        int ilosc = (rand() % 2) + 1; // Male porcje (1-2)
        int potrzebne_miejsce = ilosc * rozmiar;
        
        sem_wait(sem_id, SEM_MUTEX);
        if (!running) {
            sem_signal(sem_id, SEM_MUTEX);
            break;
        }

        int wolne_fizycznie = MAGAZYN_POJEMNOSC - mag->suma_bajtow;
        if (wolne_fizycznie < potrzebne_miejsce) {
            sem_signal(sem_id, SEM_MUTEX);

            if (czy_czekam == 0) {
                sprintf(log_buf, "%s[DOSTAWCA-%c]%s BRAK MIEJSCA DLA TEGO SKLADNIKU (%d/%d) - czekam...", 
                    KOLOR_ZOLTY, skladnik, KOLOR_RESET, mag->suma_bajtow, MAGAZYN_POJEMNOSC);
                wyslij_log(sem_id, log_buf);
                czy_czekam = 1;
            }

            usleep(10000);
            continue;
        }

        if (!czy_bezpiecznie(mag, skladnik)) {
            sem_signal(sem_id, SEM_MUTEX);

            if (czy_czekam == 0) {
                sprintf(log_buf, "%s[DOSTAWCA-%c]%s LIMIT NADPRODUKCJI - czekam...", 
                    KOLOR_ZOLTY, skladnik, KOLOR_RESET);
                wyslij_log(sem_id, log_buf);
                czy_czekam = 1;
            }

            usleep(10000);
            continue;
        }

        czy_czekam = 0;

        int wstawiono = 0;
        for (int k=0; k<ilosc; k++) {
            if (wstaw_do_kolejki(mag, skladnik)) {
                wstawiono++;
            }
        }
        
        if (wstawiono > 0) {
            sprintf(log_buf, "%s[DOSTAWCA-%c]%s Dostarczono %s%d x %c%s | Magazyn zajety: %d/%d |", 
                    KOLOR_ZIELONY, skladnik, KOLOR_RESET,
                    KOLOR_BOLD, wstawiono, skladnik, KOLOR_RESET,
                    mag->suma_bajtow, MAGAZYN_POJEMNOSC);
            wyslij_log(sem_id, log_buf);
        }

        sem_signal(sem_id, SEM_MUTEX);
        
        for (int j = 0; j < wstawiono; j++) {
            sem_signal(sem_id, sem_skladnik);
        }
        
        sleep((rand() % 3) + 1);
    }
    
    sprintf(log_buf, "%s[DOSTAWCA-%c]%s Koniec pracy", 
            KOLOR_CZERWONY, skladnik, KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}