/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#define BUFFER_SZ 150
#define N_FILES 100 //numero massimo di file da elaborato
#define MAX_PATH 250
#define MAX_PORTION 1050

struct mymsg{
    long mtype; //lo usiamo per memorizzare il PID
    int pid;
    char portion[MAX_PORTION];
    char pathname[MAX_PATH];
};

struct myfile{
    int pid;
    char pathname[MAX_PATH];
    char fifo1[MAX_PORTION];
    char fifo2[MAX_PORTION];
    char msgQ[MAX_PORTION];
    char shMem[MAX_PORTION];
};

int search(char files[100][MAX_PATH], char currdir[], int n_files);
int divideBy4(int counter);
void divideString(char buff[],char sendByFIFO1[],char sendByFIFO2[],char sendByMsgQ[],char sendByShM[]);