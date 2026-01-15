#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <string.h>
#include "common.h"
#include "utils.h"



union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};




void inicjalizuj_magazyn(Magazyn* mag) {
    mag->head = 0;
    mag->tail = 0;
    mag->zajete = 0;
    mag->fabryka_dziala = 1;
    memset(mag->bufor, BAJT_PUSTY, MAGAZYN_POJEMNOSC);
}

// Funkcja pomocnicza: Zlicza ile razy wystepuje dany bajt w buforze
int zlicz_skladnik(Magazyn* mag, char typ) {
    int licznik = 0;
    for (int i = 0; i < MAGAZYN_POJEMNOSC; i++) {
        if (mag->bufor[i] == typ) {
            licznik++;
        }
    }
    return licznik;
}

// Wstawia bajty, nie nadpisujac innych (zeby nie nadpisac danych)
int wstaw_do_bufora(Magazyn* mag, char typ, int rozmiar) {
    // Sprawdzamy czy w ogole jest miejsce w liczniku
    if (mag->zajete + rozmiar > MAGAZYN_POJEMNOSC) return 0; 

    for (int i = 0; i < rozmiar; i++) {
        // Szukamy najblizszej wolnego miejsca od head
        while (mag->bufor[mag->head] != BAJT_PUSTY) {
            mag->head = (mag->head + 1) % MAGAZYN_POJEMNOSC;
        }

        // Wstawiamy skladnik na wolne miejsce
        mag->bufor[mag->head] = typ;
        
        // Przesuwamy head i licznik
        mag->head = (mag->head + 1) % MAGAZYN_POJEMNOSC;
        mag->zajete++;
    }
    return 1;
}

// Pobiera skladnik z ringu 
int pobierz_z_bufora(Magazyn* mag, char typ, int rozmiar) {
    int znaleziono = 0;

    for (int i = 0; i < MAGAZYN_POJEMNOSC; i++) {
        if (mag->bufor[i] == typ) {
            mag->bufor[i] = BAJT_PUSTY; // Kasujemy fizycznie
            mag->zajete--;
            znaleziono++;
            if (znaleziono == rozmiar) break;
        }
    }
    return (znaleziono == rozmiar);
}

// Wyswietla jak wyglada ring
void wizualizacja_bufora(Magazyn* m) {
    printf("   Bufor [");
    for (int i = 0; i < MAGAZYN_POJEMNOSC; i++) {
        printf("%c", m->bufor[i]);
    }
    printf("]\n");
}

// Wyswietla stan magazynu
void wyswietl_stan_magazynu(Magazyn* mag) {
    int count_a = zlicz_skladnik(mag, BAJT_A) / ROZMIAR_A;
    int count_b = zlicz_skladnik(mag, BAJT_B) / ROZMIAR_B;
    int count_c = zlicz_skladnik(mag, BAJT_C) / ROZMIAR_C;
    int count_d = zlicz_skladnik(mag, BAJT_D) / ROZMIAR_D;

    printf("\n");
    printf("+--------------------------------------------+\n");
    printf("|          STAN MAGAZYNU (RING BUFFER)       |\n");
    printf("+--------------------------------------------+\n");
    printf("|  Skladnik A: %3d szt. (%3d bajtow)          |\n", count_a, count_a * ROZMIAR_A);
    printf("|  Skladnik B: %3d szt. (%3d bajtow)          |\n", count_b, count_b * ROZMIAR_B);
    printf("|  Skladnik C: %3d szt. (%3d bajtow)          |\n", count_c, count_c * ROZMIAR_C);
    printf("|  Skladnik D: %3d szt. (%3d bajtow)          |\n", count_d, count_d * ROZMIAR_D);
    printf("+--------------------------------------------+\n");
    printf("|  Zajete: %4d / %4d bajtow                  |\n", mag->zajete, MAGAZYN_POJEMNOSC);
    printf("|  Wolne:  %4d bajtow                        |\n", MAGAZYN_POJEMNOSC - mag->zajete);
    printf("|  Head: %4d  Tail: %4d                     |\n", mag->head, mag->tail);
    printf("+--------------------------------------------+\n");
    wizualizacja_bufora(mag);
    printf("\n");
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

    printf("[SHM] Odloczono od pamieci dzielonej");
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
    
    //liczba wolnych jednostek
    arg.val = MAGAZYN_POJEMNOSC;
    if (semctl(sem_id, SEM_WOLNE, SETVAL, arg) == -1) {
        perror("semctl SEM_WOLNE");
        exit(EXIT_FAILURE);
    }
    
    // Semafory skladnikow - na poczatku 0 (brak skladnikow)
    arg.val = 0;
    for(int i=SEM_SKLAD_A; i<=SEM_SKLAD_D; i++) semctl(sem_id, i, SETVAL, arg);
}

void zaktualizuj_semafory(int sem_id, Magazyn* mag) {
    union semun arg;
    arg.val = MAGAZYN_POJEMNOSC - mag->zajete; 
    semctl(sem_id, SEM_WOLNE, SETVAL, arg);

    arg.val = zlicz_skladnik(mag, BAJT_A) / ROZMIAR_A; semctl(sem_id, SEM_SKLAD_A, SETVAL, arg);
    arg.val = zlicz_skladnik(mag, BAJT_B) / ROZMIAR_B; semctl(sem_id, SEM_SKLAD_B, SETVAL, arg);
    arg.val = zlicz_skladnik(mag, BAJT_C) / ROZMIAR_C; semctl(sem_id, SEM_SKLAD_C, SETVAL, arg);
    arg.val = zlicz_skladnik(mag, BAJT_D) / ROZMIAR_D; semctl(sem_id, SEM_SKLAD_D, SETVAL, arg);
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
    
    // Obliczamy ilosci do wyswietlenia
    int a = zlicz_skladnik(mag, BAJT_A) / ROZMIAR_A;
    int b = zlicz_skladnik(mag, BAJT_B) / ROZMIAR_B;
    int c = zlicz_skladnik(mag, BAJT_C) / ROZMIAR_C;
    int d = zlicz_skladnik(mag, BAJT_D) / ROZMIAR_D;
    
    printf("[PLIK] Zapisano stan. Zajete: %d/%d (A:%d B:%d C:%d D:%d)\n", 
           mag->zajete, MAGAZYN_POJEMNOSC, a, b, c, d);
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
    
    int faktycznie_zajete = 0;
        for(int i=0; i<MAGAZYN_POJEMNOSC; i++) {
            if (mag->bufor[i] != BAJT_PUSTY) {
                faktycznie_zajete++;
            }
        }
        
        // Jeśli jest rozbieżność, naprawiamy!
        if (mag->zajete != faktycznie_zajete) {
            printf("[FIX] Wykryto blad danych! Plik twierdzil %d, a fizycznie jest %d.\n", 
                   mag->zajete, faktycznie_zajete);
            printf("[FIX] Naprawiam licznik zajete...\n");
            mag->zajete = faktycznie_zajete;
        }

        printf("[PLIK] Odczytano i zweryfikowano stan. Zajete: %d/%d\n", 
               mag->zajete, MAGAZYN_POJEMNOSC);
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

int utworz_kolejke() {
    int msg_id = msgget(KLUCZ_MSG, IPC_CREAT | 0600);
    if (msg_id == -1) { perror("msgget utworz"); exit(EXIT_FAILURE); }
    return msg_id;
}

int polacz_kolejke() {
    int msg_id = msgget(KLUCZ_MSG, 0);
    if (msg_id == -1) { perror("msgget polacz"); exit(EXIT_FAILURE); }
    return msg_id;
}

void usun_kolejke(int msg_id) {
    if (msg_id != -1) msgctl(msg_id, IPC_RMID, NULL);
}

// Funkcja do wysylania logow
void wyslij_log(int msg_id, const char* tekst) {
    // 1. Wypisz na ekran
    printf("%s\n", tekst);

    // 2. Wyslij do kolejki (jesli istnieje)
    if (msg_id != -1) {
        Komunikat msg;
        msg.mtype = 1; 
        strncpy(msg.tekst, tekst, 255);
        msg.tekst[255] = '\0';
        msgsnd(msg_id, &msg, sizeof(msg.tekst), IPC_NOWAIT);
    }
}