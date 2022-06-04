/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

//scorre i file, verifica che siano validi e li salva in "*files"
//TODO i file devono essere .txt
int search(char files[100][MAX_PATH], char currdir[], int n_files){

    struct dirent *dentry;
    struct stat sb; //struttura di supporto per verificare la dimensione del singolo file
    DIR *dirp = opendir(currdir);
    char dummy [MAX_PATH] = "";
    strcpy(dummy, currdir);

    /* skippa le cartelle '.' e '..'
     * cerca nelle altre cartelle i file
     * che iniziano per 'sendme_'
     * */
    while((dentry = readdir(dirp)) != NULL && n_files < 100){

        if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0){
            continue;
        }
        //TODO se bisogna ignorare i file con _out alla fine, usare la funzione strstr()
        //se è un file ed inizia con 'sendme_' otteniamo il suo path e lo carichiamo in memoria
        if((dentry->d_type == DT_REG) && (strncmp(dentry->d_name, "sendme_", strlen("sendme_")) == 0)){
            char pathname[MAX_PATH] = ""; //pathname attuale
            strcpy(pathname, dummy);
            strcat(pathname, dentry->d_name);
            stat(pathname, &sb); //ottengo info file
            if(sb.st_size <= 4096){ //TODO: oppure 4000
                //il file è minore uguale a 4KB, bisogna SISTEMARE
                strcpy(files[n_files], pathname);
                n_files++;
            }
        }
        else if(dentry->d_type == DT_DIR){

            char pathWithDir[MAX_PATH] = "";
            strcpy(pathWithDir, dummy);
            strcat(pathWithDir, dentry->d_name);
            strcat(pathWithDir, "/");
            n_files = search(files, pathWithDir, n_files); //TODO forse +
        }
    }
    return n_files;
}

/*funzione che calcola l'incremento necessario per rendere un numero
 * divisibile per 4
 */

int divideBy4(int counter){
    int incremento = 0;
    while(counter % 4 != 0){
        incremento++;
        counter++;
    }
    return incremento;
}

void divideString(char buff[],char sendByFIFO1[],char sendByFIFO2[],char sendByMsgQ[],char sendByShM[]){

    char dummy[4100] = "";
    //togliamo il carattere di end of file
    /*
    int i = 0;
    while(i < strlen(buff)){
        if(buff[i] != EOF)
            dummy[i] = buff[i];
        i++;
    }
    dummy[i] = '\0'; */
    strcpy(dummy, buff);
    int step;
    char subBuffer[4][1050] = {"", "", "", ""};
    int lunghezza = (strlen(dummy)-1); //non considero il carattere di fine stringa
    printf("strlen %u\n di %s .\n", lunghezza, dummy);
    switch (lunghezza) {
        case 0: //le stringhe son già inizializzate
            sendByFIFO1[0] = '\0';
            sendByFIFO2[0] = '\0';
            sendByMsgQ[0] = '\0';
            sendByShM[0] = '\0';
            break;
        case 1:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = '\0';
            sendByFIFO2[0] = '\0';
            sendByMsgQ[0] = '\0';
            sendByShM[0] = '\0';
            break;
        case 2:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = '\0';
            sendByFIFO2[0] = dummy[1];
            sendByFIFO2[1] = '\0';
            sendByMsgQ[0] = '\0';
            sendByShM[0] = '\0';
            break;
        case 3:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = '\0';
            sendByFIFO2[0] = dummy[1];
            sendByFIFO2[1] = '\0';
            sendByMsgQ[0] = dummy[2];
            sendByMsgQ[1] = '\0';
            sendByShM[0] = '\0';
            break;
        case 4:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = '\0';
            sendByFIFO2[0] = dummy[1];
            sendByFIFO2[1] = '\0';
            sendByMsgQ[0] = dummy[2];
            sendByMsgQ[1] = '\0';
            sendByShM[0] = dummy[3];
            sendByShM[1] = '\0';
            break;
        case 5:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = dummy[1];
            sendByFIFO1[2] = '\0';
            sendByFIFO2[0] = dummy[2];
            sendByFIFO2[1] = '\0';
            sendByMsgQ[0] = dummy[3];
            sendByMsgQ[1] = '\0';
            sendByShM[0] = dummy[4];
            sendByShM[1] = '\0';
            break;
        case 6:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = dummy[1];
            sendByFIFO1[2] = '\0';
            sendByFIFO2[0] = dummy[2];
            sendByFIFO2[1] = dummy[3];
            sendByFIFO2[2] = '\0';
            sendByMsgQ[0] = dummy[4];
            sendByMsgQ[1] = '\0';
            sendByShM[0] = dummy[5];
            sendByShM[1] = '\0';
            break;
        case 7:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = dummy[1];
            sendByFIFO1[2] = '\0';
            sendByFIFO2[0] = dummy[2];
            sendByFIFO2[1] = dummy[3];
            sendByFIFO2[2] = '\0';
            sendByMsgQ[0] = dummy[4];
            sendByMsgQ[1] = dummy[5];
            sendByMsgQ[2] = '\0';
            sendByShM[0] = dummy[6];
            sendByShM[1] = '\0';
            break;
        case 9:
            sendByFIFO1[0] = dummy[0];
            sendByFIFO1[1] = dummy[1];
            sendByFIFO1[2] = dummy[2];
            sendByFIFO1[3] = '\0';
            sendByFIFO2[0] = dummy[3];
            sendByFIFO2[1] = dummy[4];
            sendByFIFO2[2] = dummy[5];
            sendByFIFO2[3] = '\0';
            sendByMsgQ[0] = dummy[6];
            sendByMsgQ[1] = dummy[7];
            sendByMsgQ[2] = '\0';
            sendByShM[0] = dummy[8];
            sendByShM[1] = '\0';
            break;
        default:
            step = (lunghezza + divideBy4(lunghezza)) / 4;
            int offset = 0;

            for(int i = 0; i < 4; i++){
                memcpy(subBuffer[i],&dummy[offset],step);
                offset += step;
            }
            strcpy(sendByFIFO1, subBuffer[0]);
            strcpy(sendByFIFO2, subBuffer[1]);
            strcpy(sendByMsgQ, subBuffer[2]);
            strcpy(sendByShM, subBuffer[3]);
            break;
    }
}