#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "common.h"  
#include "utils.h"

pid_t pids_dostawcy[4];
pid_t pids_pracownicy[2];
int liczba_dostawcow = 0;
int liczba_pracownikow = 0;
char log_buf[256];
volatile sig_atomic_t running = 1;

// Handler dla SIGCHLD - zapobiega procesom zombie
void handle_sigchld(int sig) {
    (void)sig;
    // Zbieramy wszystkie zakonczone procesy potomne
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Handler dla SIGINT (Ctrl+C) - graceful shutdown
void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    
    // Wysylamy SIGTERM do wszystkich procesow potomnych
    for(int i=0; i<liczba_dostawcow; i++) {
        if (pids_dostawcy[i] > 0) {
            kill(pids_dostawcy[i], SIGTERM);
        }
    }
    for(int i=0; i<liczba_pracownikow; i++) {
        if (pids_pracownicy[i] > 0) {
            kill(pids_pracownicy[i], SIGTERM);
        }
    }
    
    // Wypisz komunikat na stderr (bezpieczne w handlerze sygnału)
    const char msg[] = "\n\n[DYREKTOR] Otrzymano Ctrl+C - kończę pracę...\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

void wyswietl_menu() {
    printf("\n");
    printf("%s%s====================================================\n", KOLOR_BOLD, KOLOR_NIEBIESKI);
    printf("||                  MENU DYREKTORA                ||\n");
    printf("====================================================%s\n", KOLOR_RESET);
    printf("%s||  1 - Polecenie_1: Fabryka konczy prace         ||\n", KOLOR_ZOLTY);
    printf("||  2 - Polecenie_2: Zamknij/Otworz Magazyn       ||\n");
    printf("||  3 - Polecenie_3: Dostawcy przerywaja prace    ||\n");
    printf("||  4 - Polecenie_4: Koniec + zapis stanu         ||\n");
    printf("||  5 - Polecenie_5: Wyswietl stan magazynu       ||%s\n", KOLOR_RESET);
    printf("%s%s====================================================%s\n", KOLOR_BOLD, KOLOR_NIEBIESKI, KOLOR_RESET);
    printf("%sWybierz opcje: %s", KOLOR_BOLD, KOLOR_RESET);
    fflush(stdout);
}

void posprzataj(int shm_id, int sem_id, Magazyn* mag) {
    printf("\n[DYREKTOR] Sprzatanie zasobow...\n");

    odlacz_pamiec_dzielona(mag);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
}

int main() {
    // Rejestracja handlerow sygnalow
    struct sigaction sa_chld, sa_int;
    
    // SIGCHLD - zapobiega procesom zombie
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;  // Nie przerywaj dla SIGCHLD
    sigaction(SIGCHLD, &sa_chld, NULL);
    
    // SIGINT (Ctrl+C) - graceful shutdown
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;  // BEZ SA_RESTART - przerywa scanf!
    sigaction(SIGINT, &sa_int, NULL);
    
    // Czyszczenie pliku raportu na start 
    FILE* f = fopen(PLIK_RAPORTU, "w");
    if (f) { 
        fprintf(f, "=== START ===\n"); 
        fclose(f); 
    }


    // Tymczasowe ID semafora dla pierwszych logow (zanim powstana)
    int sem_id_log = -1;
    
    sprintf(log_buf, "========================================");
    wyslij_log(sem_id_log, log_buf);
    sprintf(log_buf, "    FABRYKA CZEKOLADY - DYREKTOR");
    wyslij_log(sem_id_log, log_buf);
    sprintf(log_buf, "========================================");
    wyslij_log(sem_id_log, log_buf);
    
    // Inicjalizacja pamieci dzielonej 

    int shm_id = utworz_pamiec_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int wczytano_stan = 0;

    // Inicjalizacja magazynu
    sprintf(log_buf, "[DYREKTOR] Inicjalizacja magazynu...");
    wyslij_log(sem_id_log, log_buf);

    // Sprawdz czy istnieje zapisany stan
    if (czy_istnieje_plik_stanu(MAGAZYN_PLIK)) {
        sprintf(log_buf, "[DYREKTOR] Znaleziono zapisany stan magazynu");
        wyslij_log(sem_id_log, log_buf);
        if (odczytaj_stan_magazynu(magazyn, MAGAZYN_PLIK) == 0) {
            sprintf(log_buf, "[DYREKTOR] Stan magazynu odtworzony z pliku!");
            wyslij_log(sem_id_log, log_buf);
            wczytano_stan = 1;
        } else {
            sprintf(log_buf, "[DYREKTOR] Blad odczytu - inicjalizacja od zera");
            wyslij_log(sem_id_log, log_buf);
            inicjalizuj_magazyn(magazyn);
            wczytano_stan = 0;
        }
    } else {
        sprintf(log_buf, "[DYREKTOR] Brak zapisanego stanu - inicjalizacja od zera");
        wyslij_log(sem_id_log, log_buf);
        inicjalizuj_magazyn(magazyn);
        wczytano_stan = 0;
    }

    // Inicjalizacja semaforow
    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);
    if (wczytano_stan) {
        zaktualizuj_semafory(sem_id, magazyn);
    }
    
    // Ustawienie sem_id_log
    sem_id_log = sem_id;

    sprintf(log_buf, "\n[DYREKTOR] Poczatkowy stan magazynu:");
    wyslij_log(sem_id_log, log_buf);
    wyswietl_stan_magazynu(sem_id_log, magazyn);

    
    // Uruchamianie dostawcow
    const char* skladniki[] = {"A", "B", "C", "D"};
    for (int i = 0; i < 4; i++) {
        pids_dostawcy[liczba_dostawcow] = fork();
        if (pids_dostawcy[liczba_dostawcow] == 0) {
            execl("./bin/dostawca", "dostawca", skladniki[i], NULL);
            perror("execl dostawca");
            exit(1);
    }
        sprintf(log_buf, "[DYREKTOR] Dostawca %s uruchomiony (PID: %d)", 
                skladniki[i], pids_dostawcy[liczba_dostawcow]);
        wyslij_log(sem_id_log, log_buf);
        liczba_dostawcow++;
    }

    // Uruchamianie pracownikow
    sprintf(log_buf, "\n[DYREKTOR] Uruchamiam 2 pracownikow (stanowisko 1 i 2)...");

    for (int i = 1; i <= 2; i++) {
        pids_pracownicy[liczba_pracownikow] = fork();
        if (pids_pracownicy[liczba_pracownikow] == 0) {
            char stanowisko[2];
            snprintf(stanowisko, sizeof(stanowisko), "%d", i);
            execl("./bin/pracownik", "pracownik", stanowisko, NULL);
            perror("execl pracownik");
            exit(1);
    }
    sprintf(log_buf, "[DYREKTOR] Pracownik %d uruchomiony (PID: %d)", 
           i, pids_pracownicy[liczba_pracownikow]);
    wyslij_log(sem_id_log, log_buf);
    liczba_pracownikow++;
    }

    sprintf(log_buf, "\n[DYREKTOR] Fabryka uruchomiona!");

    // --- MENU GLOWNE ---
    int opcja;
    

    while(running) {
        // Sprawdz czy proces nie zostal przerwany przez sygnal
        if (!running) {
            sprintf(log_buf, "\n%s[DYREKTOR]%s Otrzymano sygnal zakonczenia (Ctrl+C)", 
                    KOLOR_CZERWONY, KOLOR_RESET);
            wyslij_log(sem_id_log, log_buf);
            break;
        }
        
        wyswietl_menu();

        // Walidacja wejscia od uzytkownika
        if (scanf("%d", &opcja) != 1) {
            // Sprawdz czy scanf zostal przerwany przez sygnal
            if (!running) {
                break;  // Ctrl+C przerwał scanf
            }
            sprintf(log_buf, "\n%s[BLAD]%s Wprowadz liczbe calkowita!", KOLOR_CZERWONY, KOLOR_RESET);
            wyslij_log(sem_id_log, log_buf);
            while(getchar() != '\n'); // Czyszczenie bufora wejscia
            continue;
        }
        
        // Sprawdz czy nie otrzymalismy sygnalu podczas scanf
        if (!running) {
            break;
        }
        
        // Sprawdzenie zakresu opcji
        if (opcja < 1 || opcja > 5) {
            sprintf(log_buf, "\n%s[BLAD]%s Opcja musi byc z zakresu 1-5! Wprowadzono: %d", 
                    KOLOR_CZERWONY, KOLOR_RESET, opcja);
            wyslij_log(sem_id_log, log_buf);
            continue;
        }

        switch(opcja) {
            case 1: // Stop Pracownikow
                sprintf(log_buf, "%s>> [DYREKTOR] Wysylam SIGUSR1 do Pracownikow...%s", KOLOR_ZOLTY, KOLOR_RESET);
                wyslij_log(sem_id_log, log_buf);
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGUSR1);
                break;
            
            case 2: // Zamknij/Otworz Magazyn
                sem_wait(sem_id, SEM_MUTEX);
                if (magazyn->magazyn_otwarty) {
                    magazyn->magazyn_otwarty = 0;
                    sprintf(log_buf, "%s>> [DYREKTOR] Magazyn ZAMKNIETY - blokada operacji!%s", 
                            KOLOR_CZERWONY, KOLOR_RESET);
                } else {
                    magazyn->magazyn_otwarty = 1;
                    sprintf(log_buf, "%s>> [DYREKTOR] Magazyn OTWARTY - wznowiono operacje!%s", 
                            KOLOR_ZIELONY, KOLOR_RESET);
                }
                sem_signal(sem_id, SEM_MUTEX);
                wyslij_log(sem_id_log, log_buf);
                break;

            case 3: // Stop Dostawcow
                sprintf(log_buf, "%s>> [DYREKTOR] Wysylam SIGUSR2 do Dostawcow...%s", KOLOR_ZOLTY, KOLOR_RESET);
                wyslij_log(sem_id_log, log_buf);
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGUSR2);
                break;
            
            case 4: // Stop wszystkich + zapis stanu
                sprintf(log_buf, "%s>> [DYREKTOR] Koniec symulacji. Zatrzymuje wszystkich...%s", KOLOR_CZERWONY, KOLOR_RESET);
                wyslij_log(sem_id_log, log_buf);
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGTERM);
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGTERM);
                zapisz_stan_magazynu(magazyn, MAGAZYN_PLIK);
                running = 0;
                break;
            
            case 5:
                wyswietl_stan_magazynu(sem_id_log, magazyn);
                break;
            default: 
                sprintf(log_buf, "Nieznana opcja.");
                wyslij_log(sem_id_log, log_buf);
        }
    }

    // Zapisz stan magazynu jeśli przerwano przez Ctrl+C
    if (!running) {
        sprintf(log_buf, "\n%s[DYREKTOR]%s Przerwano przez Ctrl+C - zapisuje stan magazynu...", 
                KOLOR_ZOLTY, KOLOR_RESET);
        wyslij_log(sem_id_log, log_buf);
        //zapisz_stan_magazynu(magazyn, MAGAZYN_PLIK);
    }
    
    // Czekaj na wszystkie procesy potomne
    sprintf(log_buf, "[DYREKTOR] Czekam na zakonczenie procesow potomnych...");
    wyslij_log(sem_id_log, log_buf);
    
    for(int i=0; i<liczba_dostawcow; i++) {
        if (pids_dostawcy[i] > 0) {
            waitpid(pids_dostawcy[i], NULL, 0);
        }
    }
    for(int i=0; i<liczba_pracownikow; i++) {
        if (pids_pracownicy[i] > 0) {
            waitpid(pids_pracownicy[i], NULL, 0);
        }
    }
    
    sprintf(log_buf, "[DYREKTOR] Wszystkie procesy potomne zakonczone");
    wyslij_log(sem_id_log, log_buf);
    
    wyswietl_stan_magazynu(sem_id_log, magazyn);
    posprzataj(shm_id, sem_id, magazyn);

    return 0;
}