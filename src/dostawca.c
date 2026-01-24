#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/sem.h>
#include <string.h>
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
    signal(SIGUSR2, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);   // Obsluga Ctrl+C

    char skladnik = argv[1][0];
    int rozmiar; 
    int sem_skladnik;
    int sem_limit;

    switch(skladnik) {
    case 'A':
        rozmiar = ROZMIAR_A;
        sem_skladnik = SEM_SKLAD_A;
        sem_limit = SEM_LIMIT_A;
        break;
    case 'B':
        rozmiar = ROZMIAR_B;
        sem_skladnik = SEM_SKLAD_B;
        sem_limit = SEM_LIMIT_B;
        break;
    case 'C':
        rozmiar = ROZMIAR_C;
        sem_skladnik = SEM_SKLAD_C;
        sem_limit = SEM_LIMIT_C;
        break;
    case 'D':
        rozmiar = ROZMIAR_D;
        sem_skladnik = SEM_SKLAD_D;
        sem_limit = SEM_LIMIT_D;
        break;
    default:
        fprintf(stderr, "Nieprawidlowy skladnik: %c\n", skladnik);
        return 1;
    }


    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* mag = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
    
    char log_buf[256];

    sprintf(log_buf, "%s[DOSTAWCA-%c]%s PID:%d Start pracy (rozmiar jednostki: %d)", 
            KOLOR_ZIELONY, skladnik, KOLOR_RESET, getpid(), rozmiar);
    wyslij_log(sem_id, log_buf);
    
    srand(time(NULL) + getpid());

    while (running) {
        // Sprawdz czy magazyn jest otwarty
        if (!mag->magazyn_otwarty) {
            sprintf(log_buf, "%s[DOSTAWCA-%c]%s Magazyn zamkniety - czekam...", 
                    KOLOR_CZERWONY, skladnik, KOLOR_RESET);
            wyslij_log(sem_id, log_buf);
            sleep(2);
            continue;
        }
        
        int ilosc = (rand() % 2) + 1; // Male porcje (1-2) (Jesli MAGAZYN_POJEMNOSC <=13 nalezy ustawic na 1)
        int potrzebne_miejsce = ilosc * rozmiar;
        
        // Tworzymy tablice operacji dla semop
        struct sembuf czekaj[2];
        
        // Sprawdz czy sa wolne bajty w calym magazynie
        czekaj[0].sem_num = SEM_WOLNE;
        czekaj[0].sem_op = -potrzebne_miejsce;
        czekaj[0].sem_flg = 0;

        // Sprawdz czy jest wolne miejsce w limicie sztuk dla tego skladnika
        czekaj[1].sem_num = sem_limit;
        czekaj[1].sem_op = -ilosc;
        czekaj[1].sem_flg = 0;

        int wolne_bajty = semctl(sem_id, SEM_WOLNE, GETVAL);
        int wolne_sloty = semctl(sem_id, sem_limit, GETVAL);
        
        if (wolne_bajty < potrzebne_miejsce || wolne_sloty < ilosc) {
            sprintf(log_buf, "%s[DOSTAWCA-%c]%s Brak miejsca w magazynie. Czekam...", 
                    KOLOR_ZOLTY, skladnik, KOLOR_RESET);
            wyslij_log(sem_id, log_buf);
        }

        if (semop(sem_id, czekaj, 2) == -1) {
            if (!running) break;
            // Jesli to nie sygnal zakonczenia - sprobuj ponownie
            continue;
        }

        // Sekcja krytyczna
        sem_wait(sem_id, SEM_MUTEX);
        if (!running) {
            sem_signal(sem_id, SEM_MUTEX);
            break;
        }

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
        
        // Sygnalizujemy dostepnosc towaru
        struct sembuf signal_op;
        signal_op.sem_num = sem_skladnik;
        signal_op.sem_op = wstawiono;
        signal_op.sem_flg = 0;
        semop(sem_id, &signal_op, 1);
        
        sleep((rand() % 3) + 1);
    }
    
    sprintf(log_buf, "%s[DOSTAWCA-%c]%s Koniec pracy", 
            KOLOR_CZERWONY, skladnik, KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}