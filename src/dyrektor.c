#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "common.h"  
#include "utils.h"

pid_t pids_dostawcy[4];
pid_t pids_pracownicy[2];
pid_t pid_loger;
int liczba_dostawcow = 0;
int liczba_pracownikow = 0;

void wyswietl_menu() {
    printf("\n");
    printf("====================================================\n");
    printf("||                  MENU DYREKTORA                ||\n");
    printf("====================================================\n");
    printf("||  1 - Polecenie_1: Fabryka konczy prace         ||\n");
    printf("||  2 - Polecenie_2: Zamknij Magazyn              ||\n");
    printf("||  3 - Polecenie_3: Dostawcy przerywaja prace    ||\n");
    printf("||  4 - Polecenie_4: Fabryka+Magazyn konczy prace ||\n");
    printf("||  5 - Polecenie_5: Wyswietl stan magazynu       || \n");
    printf("====================================================\n");
    printf("Wybierz opcje: ");
    fflush(stdout);
}

void posprzataj(int shm_id, int sem_id, int msg_id, Magazyn* mag) {
    printf("\n[DYREKTOR] Sprzatanie zasobow...\n");

    // Zabij logera
    if (pid_loger > 0) {
        kill(pid_loger, SIGTERM);
        waitpid(pid_loger, NULL, 0);
    }

    // Usun kolejke
    usun_kolejke(msg_id);
    odlacz_pamiec_dzielona(mag);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
}

void proces_logera() {
    printf("[LOGER] Start. Zapisuje do pliku: %s\n", PLIK_RAPORTU);
    int msg_id = utworz_kolejke();
    
    FILE* f = fopen(PLIK_RAPORTU, "w");
    if (!f) { perror("fopen raport"); exit(1); }
    
    fprintf(f, "=== RAPORT SYMULACJI ===\n");
    fflush(f);
    
    Komunikat msg;
    while(1) {
        // msgrcv blokuje proces (nie zuzywa CPU), czeka na wiadomosc
        if (msgrcv(msg_id, &msg, sizeof(msg.tekst), 1, 0) != -1) {
            fprintf(f, "%s\n", msg.tekst);
            fflush(f);
        }
    }
}

int main() {

    printf("========================================\n");
    printf("    FABRYKA CZEKOLADY - DYREKTOR\n");
    printf("========================================\n\n");

    int shm_id = utworz_pamiec_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int wczytano_stan = 0;

    // Inicjalizacja magazynu
    printf("[DYREKTOR] Inicjalizacja magazynu...\n");

    // Sprawdz czy istnieje zapisany stan
    if (czy_istnieje_plik_stanu(MAGAZYN_PLIK)) {
        printf("[DYREKTOR] Znaleziono zapisany stan magazynu\n");
        if (odczytaj_stan_magazynu(magazyn, MAGAZYN_PLIK) == 0) {
            printf("[DYREKTOR] Stan magazynu odtworzony z pliku!\n");
            wczytano_stan = 1;
        } else {
            printf("[DYREKTOR] Blad odczytu - inicjalizacja od zera\n");
            inicjalizuj_magazyn(magazyn);
            wczytano_stan = 0;
        }
    } else {
        printf("[DYREKTOR] Brak zapisanego stanu - inicjalizacja od zera\n");
        inicjalizuj_magazyn(magazyn);
        wczytano_stan = 0;
    }

    printf("\n[DYREKTOR] Poczatkowy stan magazynu:\n");
    wyswietl_stan_magazynu(magazyn);

    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);
    if (wczytano_stan) {
        zaktualizuj_semafory(sem_id, magazyn);
    }

    // Uruchamianie logera
    if ((pid_loger = fork()) == 0) {
        proces_logera();
        exit(0);
    }


    // Uruchamianie dostawcow
    const char* skladniki[] = {"A", "B", "C", "D"};
    for (int i = 0; i < 4; i++) {
        pids_dostawcy[liczba_dostawcow] = fork();
        if (pids_dostawcy[liczba_dostawcow] == 0) {
            execl("./bin/dostawca", "dostawca", skladniki[i], NULL);
            perror("execl dostawca");
            exit(1);
    }
        printf("[DYREKTOR] Dostawca %s uruchomiony (PID: %d)\n", 
                skladniki[i], pids_dostawcy[liczba_dostawcow]);
        liczba_dostawcow++;
    }

    // Uruchamianie pracownikow
    printf("\n[DYREKTOR] Uruchamiam 2 pracownikow (stanowisko 1 i 2)...\n");

    for (int i = 1; i <= 2; i++) {
        pids_pracownicy[liczba_pracownikow] = fork();
        if (pids_pracownicy[liczba_pracownikow] == 0) {
            char stanowisko[2];
            snprintf(stanowisko, sizeof(stanowisko), "%d", i);
            execl("./bin/pracownik", "pracownik", stanowisko, NULL);
            perror("execl pracownik");
            exit(1);
    }
    printf("[DYREKTOR] Pracownik %d uruchomiony (PID: %d)\n", 
           i, pids_pracownicy[liczba_pracownikow]);
    liczba_pracownikow++;
    }

    printf("\n[DYREKTOR] Fabryka uruchomiona!\n");

    // --- MENU GLOWNE ---
    int running = 1;
    int opcja;
    

    while(running) {
        wyswietl_menu();


        if (scanf("%d", &opcja) != 1) { while(getchar()!='\n'); continue; }

        switch(opcja) {
            case 1: // Stop Pracownikow
                printf(">> Wysylam SIGUSR1 do Pracownikow...\n");
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGUSR1);
                break;

            case 3: // Stop Dostawcow
                printf(">> Wysylam SIGUSR2 do Dostawcow...\n");
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGUSR2);
                break;
            
            case 2: // Stop Magazynu (Wszyscy)
            case 4: // Stop
                printf(">> Koniec symulacji. Zatrzymuje wszystkich...\n");
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGTERM);
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGTERM);
                if (opcja == 4) {
                    zapisz_stan_magazynu(magazyn, MAGAZYN_PLIK);
                }

                running = 0;
                break;
            
            case 5:
                wyswietl_stan_magazynu(magazyn);
                break;
            default: printf("Nieznana opcja.\n");
        }
    }

    // Czekaj na dzieci
    for(int i=0; i<6; i++) wait(NULL);
    
    wyswietl_stan_magazynu(magazyn);
    int msg_id = polacz_kolejke();
    posprzataj(shm_id, sem_id, msg_id, magazyn);

    return 0;
}