/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

struct mymsg{
    long mtype; //lo usiamo per memorizzare il PID
    char *portion;
    char *pathname;
};

struct myfile{
    long pid;
    char *pathname;
    char *fifo1;
    char *fifo2;
    char *msgQ;
    char *shMem;
};

int search(char files[100][256], char *currdir);
int divideBy4(int counter);
void divideString(char buff[],char sendByFIFO1[],char sendByFIFO2[],char sendByMsgQ[],char sendByShM[]);