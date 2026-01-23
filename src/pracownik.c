#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
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

    int stanowisko = atoi(argv[1]);
    const char* typ_czekolady = (stanowisko == 1) ? "TYP_1 (A+B+C)" : "TYP_2 (A+B+D)";

    
    int shm_id = polacz_magazyn_z_pamiecia_dzielona();
    Magazyn* mag = polacz_z_pamiecia_dzielona(shm_id);
    int sem_id = polacz_semafory();
   
    char log_buf[256];

    sprintf(log_buf, "%s[PRACOWNIK-%d]%s PID:%d Start - produkuje czekolade %s", 
            KOLOR_CYAN, stanowisko, KOLOR_RESET, getpid(), typ_czekolady);
    wyslij_log(sem_id, log_buf); 

    srand(time(NULL) + getpid());
    int wyprodukowano = 0;

    // Definiujemy operacje wait - czekanie na wszystkie skladniki
    struct sembuf czekaj[3];
    czekaj[0].sem_num = SEM_SKLAD_A; czekaj[0].sem_op = -1; czekaj[0].sem_flg = 0;
    czekaj[1].sem_num = SEM_SKLAD_B; czekaj[1].sem_op = -1; czekaj[1].sem_flg = 0;
    
    if (stanowisko == 1) {
        czekaj[2].sem_num = SEM_SKLAD_C;
    } else {
        czekaj[2].sem_num = SEM_SKLAD_D;
    }
    czekaj[2].sem_op = -1; czekaj[2].sem_flg = 0;

    
    while (running) {

        // Atomowe pobieranie skladnikow A, B i C/D jednoczesnie
        if (semop(sem_id, czekaj, 3) == -1) {
             if (!running) break;
             continue;
        }

        sem_wait(sem_id, SEM_MUTEX);
        if (!running) {
            sem_signal(sem_id, SEM_MUTEX);
            break;
        }
        
        // Pobranie danych
        char a = (char)pobierz_z_kolejki(mag, BAJT_A);
        char b = (char)pobierz_z_kolejki(mag, BAJT_B);
        char c_or_d;
        int zwolnione_bajty = ROZMIAR_A + ROZMIAR_B;
        
        if (stanowisko == 1) {
            c_or_d = (char)pobierz_z_kolejki(mag, BAJT_C);
            zwolnione_bajty += ROZMIAR_C;
        } else {
            c_or_d = (char)pobierz_z_kolejki(mag, BAJT_D);
            zwolnione_bajty += ROZMIAR_D;
        }

        sprintf(log_buf, "%s[PRACOWNIK-%d]%s Pobrano: %c, %c, %c | Magazyn zajety: %d/%d |", 
                KOLOR_CYAN, stanowisko, KOLOR_RESET, a, b, c_or_d,
                mag->suma_bajtow, MAGAZYN_POJEMNOSC);

        wyslij_log(sem_id, log_buf);

        sem_signal(sem_id, SEM_MUTEX);

        // Zwolnienie semaforow
        struct sembuf zwolnij[4];
        
        // Zwroc bajty ogolne
        zwolnij[0].sem_num = SEM_WOLNE;
        zwolnij[0].sem_op = zwolnione_bajty;
        zwolnij[0].sem_flg = 0;

        // Zwroc slot dla A
        zwolnij[1].sem_num = SEM_LIMIT_A;
        zwolnij[1].sem_op = 1;
        zwolnij[1].sem_flg = 0;

        // Zwroc slot dla B
        zwolnij[2].sem_num = SEM_LIMIT_B;
        zwolnij[2].sem_op = 1;
        zwolnij[2].sem_flg = 0;

        // Zwroc slot dla C lub D
        if (stanowisko == 1) zwolnij[3].sem_num = SEM_LIMIT_C;
        else zwolnij[3].sem_num = SEM_LIMIT_D;
        zwolnij[3].sem_op = 1;
        zwolnij[3].sem_flg = 0;

        semop(sem_id, zwolnij, 4);


        // Produkcja
        //sleep((rand() % 5) + 1);
        wyprodukowano++;
        sprintf(log_buf, "%s%s[PRACOWNIK-%d] *** WYPRODUKOWANO CZEKOLADE - %s  #%d ***%s", 
                KOLOR_BOLD, KOLOR_MAGENTA, stanowisko, typ_czekolady, wyprodukowano, KOLOR_RESET);
        wyslij_log(sem_id, log_buf);
    }

    sprintf(log_buf, "%s[PRACOWNIK-%d]%s Koniec. Wyprodukowano: %s%d%s czekolady %s", 
            KOLOR_CZERWONY, stanowisko, KOLOR_RESET, 
            KOLOR_BOLD, wyprodukowano, KOLOR_RESET, typ_czekolady);
    wyslij_log(sem_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}