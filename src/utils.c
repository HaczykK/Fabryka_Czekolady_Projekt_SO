#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/sem.h>
#include "common.h"
#include "utils.h"



union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};




void wyswietl_stan_magazynu(Magazyn* mag) {
    printf("\n\n==========================\n");
    printf("||----Stan magazynu-----||\n");
    printf("||Skladnik A: %d         ||\n", mag->skladnik_A);
    printf("||Skladnik B: %d         ||\n", mag->skladnik_B);
    printf("||Skladnik C: %d         ||\n", mag->skladnik_C);
    printf("||Skladnik D: %d         ||\n", mag->skladnik_D);
    printf("||Wolne miejsce: %d/%d  ||\n", mag->wolne_miejsce, MAGAZYN_POJEMNOSC);
    printf("==========================");
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
    for (int i = SEM_SKLAD_A; i <= SEM_SKLAD_D; i++) {
        if (semctl(sem_id, i, SETVAL, arg) == -1) {
            perror("semctl skladniki");
            exit(EXIT_FAILURE);
        }
    }
    
    printf("[SEM] Semafory zainicjalizowane:\n");
    printf("      MUTEX=%d, WOLNE=%d, A=%d, B=%d, C=%d, D=%d\n",
           1, MAGAZYN_POJEMNOSC, 0, 0, 0, 0);
}

void zaktualizuj_semafory(int sem_id, Magazyn* mag) {
    union semun arg;

    // 1. Aktualizuj WOLNE miejsce
    arg.val = mag->wolne_miejsce;
    semctl(sem_id, SEM_WOLNE, SETVAL, arg);
    
    // 2. Aktualizuj stany skladnikow (zeby pracownicy wiedzieli ze cos jest)
    arg.val = mag->skladnik_A;
    semctl(sem_id, SEM_SKLAD_A, SETVAL, arg);
    
    arg.val = mag->skladnik_B;
    semctl(sem_id, SEM_SKLAD_B, SETVAL, arg);
    
    arg.val = mag->skladnik_C;
    semctl(sem_id, SEM_SKLAD_C, SETVAL, arg);
    
    arg.val = mag->skladnik_D;
    semctl(sem_id, SEM_SKLAD_D, SETVAL, arg);
    
    printf("[DYREKTOR] Semafory zsynchronizowane (Wolne=%d, A=%d, B=%d, C=%d, D=%d)\n",
           mag->wolne_miejsce, mag->skladnik_A, mag->skladnik_B, mag->skladnik_C, mag->skladnik_D);
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
    
    printf("[PLIK] Zapisano stan magazynu do: %s\n", plik);
    printf("       A=%d B=%d C=%d D=%d Wolne=%d\n", 
           mag->skladnik_A, mag->skladnik_B, 
           mag->skladnik_C, mag->skladnik_D, 
           mag->wolne_miejsce);
    
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
    
    printf("[PLIK] Odczytano stan magazynu z: %s\n", plik);
    printf("       A=%d B=%d C=%d D=%d Wolne=%d\n", 
           mag->skladnik_A, mag->skladnik_B, 
           mag->skladnik_C, mag->skladnik_D, 
           mag->wolne_miejsce);
    
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