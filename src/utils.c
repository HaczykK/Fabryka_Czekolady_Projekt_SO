#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "utils.h"

//#if defined(__linux__)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
//#endif


// Inicjalizacja pojedynczej kolejki FIFO
void inicjalizuj_kolejke(RingQueue* q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    // Wyzerowanie bufora dla bezpieczenstwa
    memset(q->dane, 0, KOLEJKA_POJEMNOSC);
}

void inicjalizuj_magazyn(Magazyn* mag) {
    inicjalizuj_kolejke(&mag->kolejka_A);
    inicjalizuj_kolejke(&mag->kolejka_B);
    inicjalizuj_kolejke(&mag->kolejka_C);
    inicjalizuj_kolejke(&mag->kolejka_D);
    mag->suma_bajtow = 0;
}

// Pomocnicza: zwraca wskaznik do kolejki dla danego typu skladnika
RingQueue* pobierz_kolejke(Magazyn* mag, char typ) {
    switch(typ) {
        case BAJT_A: return &mag->kolejka_A;
        case BAJT_B: return &mag->kolejka_B;
        case BAJT_C: return &mag->kolejka_C;
        case BAJT_D: return &mag->kolejka_D;
        default: return NULL;
    }
}

// Pomocnicza: zwraca rozmiar skladnika
int rozmiar_skladnika(char typ) {
    switch(typ) {
        case BAJT_A: return ROZMIAR_A;
        case BAJT_B: return ROZMIAR_B;
        case BAJT_C: return ROZMIAR_C;
        case BAJT_D: return ROZMIAR_D;
        default: return 0;
    }
}

// Sprawdza czy mozna wstawic skladnik do magazynu
int czy_mozna_wstawic(Magazyn* mag, char typ) {
    RingQueue* q = pobierz_kolejke(mag, typ);
    if (q == NULL) return 0;
    
    int rozmiar = rozmiar_skladnika(typ);
    
    // Sprawdz limit kolejki
    if (q->count >= KOLEJKA_POJEMNOSC) return 0;
    
    // Sprawdz limit calkowitej pojemnosci magazynu
    if (mag->suma_bajtow + rozmiar > MAGAZYN_POJEMNOSC) return 0;
    
    return 1;
}

// Wstawia skladnik do odpowiedniej kolejki FIFO
int wstaw_do_kolejki(Magazyn* mag, char typ) {
    RingQueue* q = pobierz_kolejke(mag, typ);
    if (q == NULL) return 0;
    
    int rozmiar = rozmiar_skladnika(typ);
    
    // Sprawdz limity
    if (q->count >= KOLEJKA_POJEMNOSC) return 0;
    if (mag->suma_bajtow + rozmiar > MAGAZYN_POJEMNOSC) return 0;
    
    // Zapisz dane fizycznie w tablicy
    q->dane[q->head] = typ;

    // Wstaw na head (FIFO - wstawiamy na koniec)
    q->head = (q->head + 1) % KOLEJKA_POJEMNOSC;
    q->count++;
    mag->suma_bajtow += rozmiar;
    
    return 1;
}

// Pobiera skladnik z odpowiedniej kolejki FIFO
int pobierz_z_kolejki(Magazyn* mag, char typ) {
    RingQueue* q = pobierz_kolejke(mag, typ);
    if (q == NULL) return 0;
    
    // Sprawdz czy jest co pobrac
    if (q->count <= 0) return 0;
    
    int rozmiar = rozmiar_skladnika(typ);
    
    // Odczytaj dane z tablicy
    char pobrany_bajt = q->dane[q->tail];

    // Pobierz z tail (FIFO - pobieramy z poczatku)
    q->tail = (q->tail + 1) % KOLEJKA_POJEMNOSC;
    q->count--;
    mag->suma_bajtow -= rozmiar;
    
    return (int)pobrany_bajt;
}

// Zwraca liczbe skladnikow danego typu w magazynie
int zlicz_skladnik(Magazyn* mag, char typ) {
    RingQueue* q = pobierz_kolejke(mag, typ);
    if (q == NULL) return 0;
    return q->count;
}

// Wyswietla stan magazynu
void wyswietl_stan_magazynu(int sem_id, Magazyn* mag) {
    char log_buf[256];

    int count_a = mag->kolejka_A.count;
    int count_b = mag->kolejka_B.count;
    int count_c = mag->kolejka_C.count;
    int count_d = mag->kolejka_D.count;
    
    int procent = (mag->suma_bajtow * 100) / MAGAZYN_POJEMNOSC;
    const char* kolor_zapelnienia;
    if (procent < 50) kolor_zapelnienia = KOLOR_ZIELONY;
    else if (procent < 80) kolor_zapelnienia = KOLOR_ZOLTY;
    else kolor_zapelnienia = KOLOR_CZERWONY;

    sprintf(log_buf, " ");
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "%s%s+--------------------------------------------+", KOLOR_BOLD, KOLOR_CYAN);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|               STAN MAGAZYNU                |");
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "+--------------------------------------------+%s", KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|  %sSkladnik A:%s %3d szt. (%3d bajtow)         |", KOLOR_ZIELONY, KOLOR_RESET, count_a, count_a * ROZMIAR_A);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|  %sSkladnik B:%s %3d szt. (%3d bajtow)         |", KOLOR_ZIELONY, KOLOR_RESET, count_b, count_b * ROZMIAR_B);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|  %sSkladnik C:%s %3d szt. (%3d bajtow)         |", KOLOR_ZIELONY, KOLOR_RESET, count_c, count_c * ROZMIAR_C);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|  %sSkladnik D:%s %3d szt. (%3d bajtow)         |", KOLOR_ZIELONY, KOLOR_RESET, count_d, count_d * ROZMIAR_D);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "%s%s+--------------------------------------------+%s", KOLOR_BOLD, KOLOR_CYAN, KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "|  Zajete: %s%4d%s / %4d bajtow (%s%3d%%%s)         |", 
            kolor_zapelnienia, mag->suma_bajtow, KOLOR_RESET, 
            MAGAZYN_POJEMNOSC, kolor_zapelnienia, procent, KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf,"|  Wolne:  %4d bajtow                       |", MAGAZYN_POJEMNOSC - mag->suma_bajtow);
    wyslij_log(sem_id, log_buf);
    
    sprintf(log_buf, "%s%s+--------------------------------------------+%s\n", KOLOR_BOLD, KOLOR_CYAN, KOLOR_RESET);
    wyslij_log(sem_id, log_buf);
    sprintf(log_buf, "\n");
    wyslij_log(sem_id, log_buf);
}

// Pamiec dzielona

int utworz_pamiec_dzielona() {
    int shm_id = shmget(SHM_KEY, sizeof(Magazyn), IPC_CREAT | 0600);
    
    if(shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    printf("[SHM] Utworzono pamiec dzielono (ID: %d)\n", shm_id);
    return shm_id;
}

Magazyn* polacz_z_pamiecia_dzielona(int shm_id) {
    Magazyn* mag = (Magazyn*) shmat(shm_id, NULL, 0);

    if(mag == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("[SHM] Polaczono z pamiecia dzielona (ID: %d)\n", shm_id);
    return mag;

}

void odlacz_pamiec_dzielona(Magazyn* mag) {
    if(shmdt(mag) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    printf("[SHM] Odloczono od pamieci dzielonej\n");
}

void usun_pamiec_dzielona(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    printf("[SHM] Usunieto pamiec dzielona (ID: %d)\n", shm_id);

}

// Semafory

int utworz_semafory() {
    int sem_id = semget(SEM_KEY, SEM_COUNT, IPC_CREAT | 0600);
    
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    
    printf("[SEM] Utworzono zestaw semaforow (ID: %d, liczba: %d)\n", sem_id, SEM_COUNT);
    return sem_id;
}

void inicjalizuj_semafory(int sem_id) {
    union semun arg;
    
    //mutex binarny (1 = wolny)
    arg.val = 1;
    if (semctl(sem_id, SEM_MUTEX, SETVAL, arg) == -1) { 
        perror("semctl SEM_MUTEX"); 
        exit(EXIT_FAILURE); 
    }


    if (semctl(sem_id, SEM_LOG, SETVAL, arg) == -1) { 
        perror("semctl SEM_LOG"); 
        exit(EXIT_FAILURE); 
    }
    
    //liczba wolnych jednostek
    arg.val = MAGAZYN_POJEMNOSC;
    if (semctl(sem_id, SEM_WOLNE, SETVAL, arg) == -1) { 
        perror("semctl SEM_WOLNE"); 
        exit(EXIT_FAILURE); 
    }
    
    // Semafory skladnikow (FULL)
    arg.val = 0;
    for(int i=SEM_SKLAD_A; i<=SEM_SKLAD_D; i++) semctl(sem_id, i, SETVAL, arg);

    // Semafory limitow (EMPTY slots)
    arg.val = KOLEJKA_POJEMNOSC;
    if (semctl(sem_id, SEM_LIMIT_A, SETVAL, arg) == -1) { 
        perror("semctl LIMIT_A"); 
        exit(EXIT_FAILURE); 
    }

    if (semctl(sem_id, SEM_LIMIT_B, SETVAL, arg) == -1) { 
        perror("semctl LIMIT_B"); 
        exit(EXIT_FAILURE); 
    }

    if (semctl(sem_id, SEM_LIMIT_C, SETVAL, arg) == -1) { 
        perror("semctl LIMIT_C"); 
        exit(EXIT_FAILURE); 
    }

    if (semctl(sem_id, SEM_LIMIT_D, SETVAL, arg) == -1) { 
        perror("semctl LIMIT_D"); 
        exit(EXIT_FAILURE); 
    }
}

void zaktualizuj_semafory(int sem_id, Magazyn* mag) {
    union semun arg;
    arg.val = MAGAZYN_POJEMNOSC - mag->suma_bajtow; 
    semctl(sem_id, SEM_WOLNE, SETVAL, arg);

    arg.val = mag->kolejka_A.count; semctl(sem_id, SEM_SKLAD_A, SETVAL, arg);
    arg.val = mag->kolejka_B.count; semctl(sem_id, SEM_SKLAD_B, SETVAL, arg);
    arg.val = mag->kolejka_C.count; semctl(sem_id, SEM_SKLAD_C, SETVAL, arg);
    arg.val = mag->kolejka_D.count; semctl(sem_id, SEM_SKLAD_D, SETVAL, arg);

    // Aktualizujemy limity - ile pustych miejsc
    arg.val = KOLEJKA_POJEMNOSC - mag->kolejka_A.count; semctl(sem_id, SEM_LIMIT_A, SETVAL, arg);
    arg.val = KOLEJKA_POJEMNOSC - mag->kolejka_B.count; semctl(sem_id, SEM_LIMIT_B, SETVAL, arg);
    arg.val = KOLEJKA_POJEMNOSC - mag->kolejka_C.count; semctl(sem_id, SEM_LIMIT_C, SETVAL, arg);
    arg.val = KOLEJKA_POJEMNOSC - mag->kolejka_D.count; semctl(sem_id, SEM_LIMIT_D, SETVAL, arg);

    arg.val = 1; semctl(sem_id, SEM_LOG, SETVAL, arg);
    arg.val = 1; semctl(sem_id, SEM_MUTEX, SETVAL, arg);
}

void usun_semafory(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
    
    printf("[SEM] Usunieto semafory (ID: %d)\n", sem_id);
}

void sem_wait(int sem_id, int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -1;
    op.sem_flg = 0;
    
    if (semop(sem_id, &op, 1) == -1) {
        if (errno == EINTR) {
            return; 
        }
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = 1;
    op.sem_flg = 0;
    
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop signal");
        exit(EXIT_FAILURE);
    }
}

int sem_getval(int sem_id, int sem_num) {
    int val = semctl(sem_id, sem_num, GETVAL);
    if (val == -1) {
        perror("semctl GETVAL");
        exit(EXIT_FAILURE);
    }
    
    return val;
}

//Funkcje pomocnicze dla magazynu
int polacz_semafory() {
    int sem_id = semget(SEM_KEY, SEM_COUNT, 0);

    if(sem_id == -1) {
        perror("semget polacz");
        exit(EXIT_FAILURE);
    }

    return sem_id;
}

int polacz_magazyn_z_pamiecia_dzielona() {
    int shm_id = shmget(SHM_KEY, sizeof(Magazyn), 0);

    if(shm_id == -1) {
        perror("shmget polacz");
        exit(EXIT_FAILURE);
    }

    return shm_id;
}

// Funkcje zapisu/odczytu stanu magazynu do/z pliku

int zapisz_stan_magazynu(Magazyn* mag, const char* plik) {
    FILE* f = fopen(plik, "wb");
    if (f == NULL) {
        perror("fopen zapis");
        return -1;
    }
    
    size_t written = fwrite(mag, sizeof(Magazyn), 1, f);
    fclose(f);
    
    if (written != 1) {
        return -1;
    }
    
    printf("[PLIK] Zapisano stan. Magazyn zajety: %d/%d (A:%d B:%d C:%d D:%d)\n", 
           mag->suma_bajtow, MAGAZYN_POJEMNOSC, 
           mag->kolejka_A.count, mag->kolejka_B.count, 
           mag->kolejka_C.count, mag->kolejka_D.count);
    return 0;
}

int odczytaj_stan_magazynu(Magazyn* mag, const char* plik) {
    FILE* f = fopen(plik, "rb");
    if (f == NULL) {
        // Plik nie istnieje - to nie jest blad
        return -1;
    }
    
    size_t read_count = fread(mag, sizeof(Magazyn), 1, f);
    fclose(f);
    
    if (read_count != 1) {
        return -1;
    }
    
    // Weryfikacja spojnosci danych
    int faktycznie_zajete = 
        mag->kolejka_A.count * ROZMIAR_A +
        mag->kolejka_B.count * ROZMIAR_B +
        mag->kolejka_C.count * ROZMIAR_C +
        mag->kolejka_D.count * ROZMIAR_D;
    
    if (mag->suma_bajtow != faktycznie_zajete) {
        printf("[FIX] Wykryto blad danych! Plik twierdzil %d, a suma kolejek to %d.\n", mag->suma_bajtow, faktycznie_zajete);
        printf("[FIX] Naprawiam licznik suma_bajtow...\n");
        mag->suma_bajtow = faktycznie_zajete;
    }

    printf("[PLIK] Odczytano i zweryfikowano stan. Zajete: %d/%d\n", mag->suma_bajtow, MAGAZYN_POJEMNOSC);
    return 0;
}

int czy_istnieje_plik_stanu(const char* plik) {
    FILE* f = fopen(plik, "rb");
    if (f == NULL) {
        return 0;  // Nie istnieje
    }
    fclose(f);
    return 1;  // Istnieje
}

// Funkcja do wysylania logow 
void wyslij_log(int sem_id, const char* tekst) {
    // Pobierany aktualny czas
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Formatowanie czasu
    char czas_str[32];
    strftime(czas_str, sizeof(czas_str), "%H:%M:%S", t);

    // Pelna wiadomosc do wyslania
    char pelna_wiadomosc[512];
    char tekst_clean[400];
    strncpy(tekst_clean, tekst, sizeof(tekst_clean) - 1);
    tekst_clean[sizeof(tekst_clean) - 1] = '\0';
    size_t len = strlen(tekst_clean);
    if (len > 0 && tekst_clean[len-1] == '\n') {
        tekst_clean[len-1] = '\0';
    }

    // Polacz czas i tekst
    snprintf(pelna_wiadomosc, sizeof(pelna_wiadomosc), "[%s] %s", czas_str, tekst_clean);

    // Wypisz na ekran
    printf("%s\n", pelna_wiadomosc);
    fflush(stdout);

    // Zapisz do pliku chronionego semaforem
    if (sem_id != -1) {
        sem_wait(sem_id, SEM_LOG);
        FILE* f = fopen(PLIK_RAPORTU, "a");
        if (f) {
            fprintf(f, "%s\n", pelna_wiadomosc);
            fclose(f);
        } else {
            perror("Blad otwarcia raportu");
        }

        sem_signal(sem_id, SEM_LOG);
    }
}