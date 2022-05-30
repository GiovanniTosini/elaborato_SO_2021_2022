/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

union semun{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void semOp(int semid,unsigned short sem_num, short sem_op);
