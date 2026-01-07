#ifndef UTILS_H
#define UTILS_H

#include "common.h"

// Deklaracja funkcji
void wyswietl_stan_magazynu(Magazyn* mag);


//Pamiec dzielona
int utworz_pamiec_dzielona();
Magazyn* polacz_z_pamiecia_dzielona(int shm_id);
void odlacz_pamiec_dzielona(Magazyn* mag);
void usun_pamiec_dzielona(int shm_id);


#endif