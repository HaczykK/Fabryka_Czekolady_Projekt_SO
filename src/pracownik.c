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
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
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
        
        sprintf(log_buf, "[PRACOWNIK-%d] Pobrano skladniki | Magazyn: A=%d B=%d C=%d D=%d | Wolne:%d/%d|\n",
                stanowisko,
                magazyn->skladnik_A, magazyn->skladnik_B,
                magazyn->skladnik_C, magazyn->skladnik_D,
                magazyn->wolne_miejsce, MAGAZYN_POJEMNOSC);
        

        wyslij_log(msg_id, log_buf);

        sem_signal(sem_id, SEM_MUTEX);
        
        // Zwolnij miejsce (Sygnal WOLNE)
        for(int i=0; i<zwolnione_miejsce; i++) sem_signal(sem_id, SEM_WOLNE);

        // Produkcja
        sleep((rand() % 2) + 1);
        wyprodukowano++;
        sprintf(log_buf, "[PRACOWNIK-%d] *** WYPRODUKOWANO CZEKOLADE - %s  #%d ***\n", stanowisko, typ_czekolady, wyprodukowano);
        wyslij_log(msg_id, log_buf);
    }

    sprintf(log_buf, "[PRACOWNIK-%d] Koniec. Wyprodukowano: %d czekolady %s\n", stanowisko, wyprodukowano,typ_czekolady);
    wyslij_log(msg_id, log_buf);
    odlacz_pamiec_dzielona(magazyn);
    return 0;
}