/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

struct mymsg{
    long mtype; //lo usiamo per memorizzare il PID
    int pid;
    char portion[1050];
    char pathname[4096];
};

struct myfile{
    int pid;
    char pathname[4096];
    char fifo1[1050];
    char fifo2[1050];
    char msgQ[1050];
    char shMem[1050];
};

int search(char files[100][256], char *currdir);
int divideBy4(int counter);
void divideString(char buff[],char sendByFIFO1[],char sendByFIFO2[],char sendByMsgQ[],char sendByShM[]);