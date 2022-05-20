/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

struct mymsg{
    long mtype; //lo usiamo per memorizzare il PID
    char *portion;
    char *pathname;
};

void search(char *files[], char currdir[], int n_files);
int divideBy4(int counter);
void divideString(char buff[],char sendByFIFO1[],char sendByFIFO2[],char sendByMsgQ[],char sendByShM[]);