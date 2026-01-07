#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "utils.h"


void wyswietl_stan_magazynu(Magazyn* mag) {
    printf("Stan magazynu");
    printf("Skladnik A: %d\n", mag->skladnik_A);
    printf("Skladnik B: %d\n", mag->skladnik_B);
    printf("Skladnik C: %d\n", mag->skladnik_C);
    printf("Skladnik D: %d\n", mag->skladnik_D);
    printf("Wolne miejsce: %d/%d\n", mag->wolne_miejsce, MAGAZYN_POJEMNOSC);
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