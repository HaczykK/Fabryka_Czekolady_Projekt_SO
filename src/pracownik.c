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

    sprintf(log_buf, "[PRACOWNIK-%d] PID:%d Start - produkuje czekolade %s\n", stanowisko, getpid(), typ_czekolady);
    wyslij_log(msg_id, log_buf); 

    srand(time(NULL) + getpid());
    int wyprodukowano = 0;

    while (running) {
        sprintf(log_buf, "[PRACOWNIK-%d] Czekam na skladniki...\n", stanowisko);
        wyslij_log(msg_id, log_buf);

        // Pobieranie skladnikow 
        sem_wait(sem_id, SEM_SKLAD_A); if(!running) break;
        sem_wait(sem_id, SEM_SKLAD_B); if(!running) break;
        
        if (stanowisko == 1) sem_wait(sem_id, SEM_SKLAD_C);
        else sem_wait(sem_id, SEM_SKLAD_D);
        if(!running) break;

        sem_wait(sem_id, SEM_MUTEX);
        
       // Odczyt z ringu
        pobierz_z_bufora(mag, BAJT_A, ROZMIAR_A);
        pobierz_z_bufora(mag, BAJT_B, ROZMIAR_B);
        
        if (stanowisko == 1) {
            pobierz_z_bufora(mag, BAJT_C, ROZMIAR_C);
        } else {
            pobierz_z_bufora(mag, BAJT_D, ROZMIAR_D);
        }

        sprintf(log_buf, "[PRACOWNIK-%d] Pobranno skladniki | Magazyn zajety: %d/%d |", 
                stanowisko, mag->zajete, MAGAZYN_POJEMNOSC);

        wyslij_log(msg_id, log_buf);

        sem_signal(sem_id, SEM_MUTEX);
        
        // Zwolnij miejsce (Sygnal WOLNE)
        int zwolnione = ROZMIAR_A + ROZMIAR_B + ((stanowisko==1)?ROZMIAR_C:ROZMIAR_D);
        for(int k=0; k<zwolnione; k++) sem_signal(sem_id, SEM_WOLNE);

        // Produkcja
        sleep((rand() % 2) + 1);
        wyprodukowano++;
        sprintf(log_buf, "[PRACOWNIK-%d] *** WYPRODUKOWANO CZEKOLADE - %s  #%d ***\n", stanowisko, typ_czekolady, wyprodukowano);
        wyslij_log(msg_id, log_buf);
    }

    sprintf(log_buf, "[PRACOWNIK-%d] Koniec. Wyprodukowano: %d czekolady %s\n", stanowisko, wyprodukowano, typ_czekolady);
    wyslij_log(msg_id, log_buf);
    odlacz_pamiec_dzielona(mag);
    return 0;
}