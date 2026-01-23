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
// pid_loger USUNIETY
int liczba_dostawcow = 0;
int liczba_pracownikow = 0;
char log_buf[256];

void wyswietl_menu() {
    printf("\n");
    printf("%s%s====================================================\n", KOLOR_BOLD, KOLOR_NIEBIESKI);
    printf("||                  MENU DYREKTORA                ||\n");
    printf("====================================================%s\n", KOLOR_RESET);
    printf("%s||  1 - Polecenie_1: Fabryka konczy prace         ||\n", KOLOR_ZOLTY);
    printf("||  2 - Polecenie_2: Zamknij Magazyn              ||\n");
    printf("||  3 - Polecenie_3: Dostawcy przerywaja prace    ||\n");
    printf("||  4 - Polecenie_4: Fabryka+Magazyn konczy prace ||\n");
    printf("||  5 - Polecenie_5: Wyswietl stan magazynu       ||%s\n", KOLOR_RESET);
    printf("%s%s====================================================%s\n", KOLOR_BOLD, KOLOR_NIEBIESKI, KOLOR_RESET);
    printf("%sWybierz opcje: %s", KOLOR_BOLD, KOLOR_RESET);
    fflush(stdout);
}

void posprzataj(int shm_id, int sem_id, Magazyn* mag) {
    printf("\n[DYREKTOR] Sprzatanie zasobow...\n");
    
    // Loger nie jest juz osobnym procesem
    
    odlacz_pamiec_dzielona(mag);
    usun_pamiec_dzielona(shm_id);
    usun_semafory(sem_id);
}

int main() {
    // Czyszczenie pliku raportu na start
    FILE* f = fopen(PLIK_RAPORTU, "w");
    if (f) {
        fprintf(f, "=== START NOWEJ SYMULACJI ===\n");
        fclose(f);
    }

    // Usunieto tworzenie kolejki komunikatow

    // Tymczasowo -1, bo semafory jeszcze nie istnieja (log tylko na ekran)
    int temp_sem_id = -1;
    
    sprintf(log_buf, "========================================");
    wyslij_log(temp_sem_id, log_buf);
    sprintf(log_buf, "    FABRYKA CZEKOLADY - DYREKTOR");
    wyslij_log(temp_sem_id, log_buf);
    sprintf(log_buf, "========================================");
    wyslij_log(temp_sem_id, log_buf);

    int shm_id = utworz_pamiec_dzielona();
    Magazyn* magazyn = polacz_z_pamiecia_dzielona(shm_id);
    int wczytano_stan = 0;

    sprintf(log_buf, "[DYREKTOR] Inicjalizacja magazynu...");
    wyslij_log(temp_sem_id, log_buf);

    // Sprawdz czy istnieje zapisany stan
    if (czy_istnieje_plik_stanu(MAGAZYN_PLIK)) {
        sprintf(log_buf, "[DYREKTOR] Znaleziono zapisany stan magazynu");
        wyslij_log(temp_sem_id, log_buf);
        if (odczytaj_stan_magazynu(magazyn, MAGAZYN_PLIK) == 0) {
            sprintf(log_buf, "[DYREKTOR] Stan magazynu odtworzony z pliku!");
            wyslij_log(temp_sem_id, log_buf);
            wczytano_stan = 1;
        } else {
            sprintf(log_buf, "[DYREKTOR] Blad odczytu - inicjalizacja od zera");
            wyslij_log(temp_sem_id, log_buf);
            inicjalizuj_magazyn(magazyn);
            wczytano_stan = 0;
        }
    } else {
        sprintf(log_buf, "[DYREKTOR] Brak zapisanego stanu - inicjalizacja od zera");
        wyslij_log(temp_sem_id, log_buf);
        inicjalizuj_magazyn(magazyn);
        wczytano_stan = 0;
    }

    // TWORZENIE SEMAFOROW (TERAZ JUZ MAMY ID)
    int sem_id = utworz_semafory();
    inicjalizuj_semafory(sem_id);
    if (wczytano_stan) {
        zaktualizuj_semafory(sem_id, magazyn);
    }
    
    // Od teraz uzywamy poprawnego sem_id do logowania
    sprintf(log_buf, "\n[DYREKTOR] Poczatkowy stan magazynu:");
    wyslij_log(sem_id, log_buf);
    wyswietl_stan_magazynu(sem_id, magazyn);


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
        wyslij_log(sem_id, log_buf);
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
    wyslij_log(sem_id, log_buf);
    liczba_pracownikow++;
    }

    sprintf(log_buf, "\n[DYREKTOR] Fabryka uruchomiona!");
    wyslij_log(sem_id, log_buf);

    // --- MENU GLOWNE ---
    int running = 1;
    int opcja;
    

    while(running) {
        wyswietl_menu(); // Usunieto argument msg_id

        // Walidacja wejscia od uzytkownika
        if (scanf("%d", &opcja) != 1) {
            sprintf(log_buf, "\n%s[BLAD]%s Wprowadz liczbe calkowita!", KOLOR_CZERWONY, KOLOR_RESET);
            wyslij_log(sem_id, log_buf);
            while(getchar() != '\n'); // Czyszczenie bufora wejscia
            continue;
        }
        
        // Sprawdzenie zakresu opcji
        if (opcja < 1 || opcja > 5) {
            sprintf(log_buf, "\n%s[BLAD]%s Opcja musi byc z zakresu 1-5! Wprowadzono: %d", 
                    KOLOR_CZERWONY, KOLOR_RESET, opcja);
            wyslij_log(sem_id, log_buf);
            continue;
        }

        switch(opcja) {
            case 1: // Stop Pracownikow
                sprintf(log_buf, "%s>> [DYREKTOR] Wysylam SIGUSR1 do Pracownikow...%s", KOLOR_ZOLTY, KOLOR_RESET);
                wyslij_log(sem_id, log_buf);
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGUSR1);
                break;

            case 3: // Stop Dostawcow
                sprintf(log_buf, "%s>> [DYREKTOR] Wysylam SIGUSR2 do Dostawcow...%s", KOLOR_ZOLTY, KOLOR_RESET);
                wyslij_log(sem_id, log_buf);
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGUSR2);
                break;
            
            case 2: // Stop Magazynu (Wszyscy)
            case 4: // Stop
                sprintf(log_buf, "%s>> [DYREKTOR] Koniec symulacji. Zatrzymuje wszystkich...%s", KOLOR_CZERWONY, KOLOR_RESET);
                wyslij_log(sem_id, log_buf);
                for(int i=0; i<4; i++) kill(pids_dostawcy[i], SIGTERM);
                for(int i=0; i<2; i++) kill(pids_pracownicy[i], SIGTERM);
                if (opcja == 4) {
                    zapisz_stan_magazynu(magazyn, MAGAZYN_PLIK);
                }

                running = 0;
                break;
            
            case 5:
                wyswietl_stan_magazynu(sem_id, magazyn);
                break;
            default: 
                sprintf(log_buf, "Nieznana opcja.");
                wyslij_log(sem_id, log_buf);
        }
    }

    // Czekaj na dzieci
    for(int i=0; i<6; i++) wait(NULL);
    
    wyswietl_stan_magazynu(sem_id, magazyn);
    posprzataj(shm_id, sem_id, magazyn);

    return 0;
}