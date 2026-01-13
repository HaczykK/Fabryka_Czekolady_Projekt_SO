#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "common.h"  
#include "utils.h"

pid_t pids_dostawcy[4];
pid_t pids_pracownicy[2];
int liczba_dostawcow = 0;
int liczba_pracownikow = 0;

void wyswietl_menu() {
    printf("\n");
    printf("========================================\n");
    printf("         MENU DYREKTORA\n");
    printf("========================================\n");
    printf("  1 - Polecenie_1: Fabryka konczy prace\n");
    printf("  2 - Polecenie_2: Zamknij Magazyn\n");
    printf("  3 - Polecenie_3: Dostawcy przerywaja prace\n");
    printf("  4 - Polecenie_4: Fabryka+Magazyn konczy prace\n");
    printf("========================================\n");
    printf("Wybierz opcje: ");
    fflush(stdout);
}

void posprzataj(int shm_id, int sem_id, Magazyn* mag) {
    printf("\n[DYREKTOR] Sprzatanie zasobow...\n");
    odlacz_pamiec_dzielona(mag);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
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
            magazyn->skladnik_A = 0;
            magazyn->skladnik_B = 0;
            magazyn->skladnik_C = 0;
            magazyn->skladnik_D = 0;
            magazyn->wolne_miejsce = MAGAZYN_POJEMNOSC;
            wczytano_stan = 0;
        }
    } else {
        printf("[DYREKTOR] Brak zapisanego stanu - inicjalizacja od zera\n");
        magazyn->skladnik_A = 0;
        magazyn->skladnik_B = 0;
        magazyn->skladnik_C = 0;
        magazyn->skladnik_D = 0;
        magazyn->wolne_miejsce = MAGAZYN_POJEMNOSC;
        wczytano_stan = 0;
    }

    printf("\n[DYREKTOR] Poczatkowy stan magazynu:\n");
    wyswietl_stan_magazynu(magazyn);

    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);
    if (wczytano_stan) {
        zaktualizuj_semafory(sem_id, magazyn);
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
                printf(">> Wysylam SIGUSR2 do Pracownikow...\n");
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGUSR2);
                break;

            case 3: // Stop Dostawcow
                printf(">> Wysylam SIGUSR1 do Dostawcow...\n");
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGUSR1);
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
                
            default: printf("Nieznana opcja.\n");
        }
    }

    // Czekaj na dzieci
    for(int i=0; i<6; i++) wait(NULL);
    
    wyswietl_stan_magazynu(magazyn);
    posprzataj(shm_id, sem_id, magazyn);
    return 0;
}