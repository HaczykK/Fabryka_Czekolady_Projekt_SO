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

// Sprawdza czy wszystkie potrzebne skladniki sa dostepne (atomowe sprawdzenie)
int skladniki_dostepne(Magazyn* mag, int stanowisko) {
    if (mag->kolejka_A.count < 1) return 0;
    if (mag->kolejka_B.count < 1) return 0;
    
    if (stanowisko == 1) {
        if (mag->kolejka_C.count < 1) return 0;
    } else {
        if (mag->kolejka_D.count < 1) return 0;
    }
    
    return 1;
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
    int msg_id = polacz_kolejke();
   
    char log_buf[256];

    sprintf(log_buf, "%s[PRACOWNIK-%d]%s PID:%d Start - produkuje czekolade %s\n", 
            KOLOR_CYAN, stanowisko, KOLOR_RESET, getpid(), typ_czekolady);
    wyslij_log(msg_id, log_buf); 

    srand(time(NULL) + getpid());
    int wyprodukowano = 0;

    while (running) {
        // Atomowe pobieranie skladnikow (try-lock pattern)
        // Zamiast czekac na semafory po kolei (ryzyko deadlock),
        // sprawdzamy dostepnosc wszystkich skladnikow naraz
        
        sem_wait(sem_id, SEM_MUTEX);
        if (!running) {
            sem_signal(sem_id, SEM_MUTEX);
            break;
        }
        
        // Sprawdz czy wszystkie skladniki dostepne
        if (!skladniki_dostepne(mag, stanowisko)) {
            sem_signal(sem_id, SEM_MUTEX);
            usleep(10000); // Krotka przerwa i sprobuj ponownie
            continue;
        }
        
        // Atomowe pobranie wszystkich skladnikow z kolejek FIFO
        pobierz_z_kolejki(mag, BAJT_A);
        pobierz_z_kolejki(mag, BAJT_B);
        
        if (stanowisko == 1) {
            pobierz_z_kolejki(mag, BAJT_C);
        } else {
            pobierz_z_kolejki(mag, BAJT_D);
        }

        sprintf(log_buf, "%s[PRACOWNIK-%d]%s Pobrano skladniki | Magazyn zajety: %d/%d |", 
                KOLOR_CYAN, stanowisko, KOLOR_RESET,
                mag->suma_bajtow, MAGAZYN_POJEMNOSC);

        wyslij_log(msg_id, log_buf);

        sem_signal(sem_id, SEM_MUTEX);


        sem_wait(sem_id, SEM_SKLAD_A);
        sem_wait(sem_id, SEM_SKLAD_B);

        if (stanowisko == 1) {
            sem_wait(sem_id, SEM_SKLAD_C);
        } else {
            sem_wait(sem_id, SEM_SKLAD_D);
        }

        // Produkcja
        sleep((rand() % 2) + 1);
        wyprodukowano++;
        sprintf(log_buf, "%s%s[PRACOWNIK-%d] *** WYPRODUKOWANO CZEKOLADE - %s  #%d ***%s\n", 
                KOLOR_BOLD, KOLOR_MAGENTA, stanowisko, typ_czekolady, wyprodukowano, KOLOR_RESET);
        wyslij_log(msg_id, log_buf);
    }

    sprintf(log_buf, "%s[PRACOWNIK-%d]%s Koniec. Wyprodukowano: %s%d%s czekolady %s\n", 
            KOLOR_CZERWONY, stanowisko, KOLOR_RESET, 
            KOLOR_BOLD, wyprodukowano, KOLOR_RESET, typ_czekolady);
    wyslij_log(msg_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}
