/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/msg.h>

//scorre i file, verifica che siano validi e li salva in "*files"
//TODO i file devono essere .txt
int search(char files[100][256], char currdir[], int n_files){

    struct dirent *dentry;
    struct stat sb; //struttura di supporto per verificare la dimensione del singolo file
    DIR *dirp = opendir(currdir);
    //strcat(currdir, "/");
    char dummy [256] = "";
    strcpy(dummy, currdir);

    /* skippa le cartelle '.' e '..'
     * cerca nelle altre cartelle i file
     * che iniziano per 'sendme_'
     * */
    while((dentry = readdir(dirp)) != NULL){

        if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0){
            continue;
        }
        //se è un file ed inizia con 'sendme_' otteniamo il suo path e lo carichiamo in memoria
        if((dentry->d_type == DT_REG) && (strncmp(dentry->d_name, "sendme_", strlen("sendme_")) == 0)){
            char pathname[256] = ""; //pathname attuale
            strcpy(pathname, dummy);
            strcat(pathname, dentry->d_name);
            stat(pathname, &sb); //ottengo info file
            if(sb.st_size <= 4096){ //TODO: oppure 4000
                //il file è minore uguale a 4KB, bisogna SISTEMARE
                strcpy(files[n_files], pathname);
                n_files++;
            }
            //strcpy(dummy, currdir);
        }else if(dentry->d_type == DT_DIR){

            strcat(dummy, dentry->d_name);
            char dir[256] = "";
            strcpy(dir, dummy);
            n_files = search(files, dir, n_files);
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
   
    char subBuffer[4][1050];
    int lunghezza=strlen(buff);
    int step=(lunghezza+divideBy4(lunghezza))/4;
    int offset=0;

    for(int i=0;i<4;i++){
        memcpy(subBuffer[i],&buff[offset],step);
        offset += step;
    }
    strcpy(sendByFIFO1, subBuffer[0]);
    strcpy(sendByFIFO2, subBuffer[1]);
    strcpy(sendByMsgQ, subBuffer[2]);
    strcpy(sendByShM, subBuffer[3]);
}
