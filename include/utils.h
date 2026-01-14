#ifndef UTILS_H
#define UTILS_H

#include "common.h"

// Wyswietlanie stanu magazynu
void wyswietl_stan_magazynu(Magazyn* mag);

// Pamiec dzielona
int utworz_pamiec_dzielona();
Magazyn* polacz_z_pamiecia_dzielona(int shm_id);
void odlacz_pamiec_dzielona(Magazyn* mag);
void usun_pamiec_dzielona(int shm_id);

// Semafory
int utworz_semafory();
void inicjalizuj_semafory(int sem_id);
void zaktualizuj_semafory(int sem_id, Magazyn* mag);
void usun_semafory(int sem_id);
void sem_wait(int sem_id, int sem_num);
void sem_signal(int sem_id, int sem_num);
int sem_getval(int sem_id, int sem_num);

// Pomocnicze funkcje
int polacz_semafory();
int polacz_magazyn_z_pamiecia_dzielona();

// Odczyt i zapis stanu magazynu
int zapisz_stan_magazynu(Magazyn* mag, const char* plik);
int odczytaj_stan_magazynu(Magazyn* mag, const char* plik);
int czy_istnieje_plik_stanu(const char* plik);

// Kolejka komunikatow
int utworz_kolejke();
int polacz_kolejke();
void usun_kolejke(int msg_id);

// Ta funkcja zastepuje printf - wysyla na ekran I do pliku (przez kolejke)
void wyslij_log(int msg_id, const char* tekst);

#endif