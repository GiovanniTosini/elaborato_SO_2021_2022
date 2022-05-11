/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

struct mymsg{
    long mtype;
    char *portion;
} msg;
void search(char *files[], char currdir[], int n_files);
int divideBy4(int counter);
