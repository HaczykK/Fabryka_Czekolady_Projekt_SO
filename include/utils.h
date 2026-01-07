#ifndef UTILS_H
#define UTILS_H

#include "common.h"


void wyswietl_stan_magazynu(Magazyn* mag);


//Pamiec dzielona
int utworz_pamiec_dzielona();
Magazyn* polacz_z_pamiecia_dzielona(int shm_id);
void odlacz_pamiec_dzielona(Magazyn* mag);
void usun_pamiec_dzielona(int shm_id);

// Semafory
int utworz_semafory();
void inicjalizuj_semafory(int sem_id);
void usun_semafory(int sem_id);
void sem_wait(int sem_id, int sem_num);
void sem_signal(int sem_id, int sem_num);
int sem_getval(int sem_id, int sem_num);

#endif