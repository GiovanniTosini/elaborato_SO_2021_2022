/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "defines.h"
#include "err_exit.h"
#define BUFFER_SZ 150

char currdir[BUFFER_SZ];

void sigHandler(int sig) {
    if(sig == SIGUSR1){
        printf("Termination of Client_0 process.\n");
        if(kill(getpid(), SIGTERM) == -1){
            ErrExit("Failed to terminate client_0");
        }
    }
    else if(sig == SIGINT) {
        printf("Client_0 waiting...\n");
    }
}
/*
size_t append2Path(char *directory){
    size_t lastPathEnd = strlen(currdir);
    strcat(strcat(&currdir[lastPathEnd],"/"),directory);
    return lastPathEnd;
}
*/


int main(int argc, char * argv[]) {

    struct dirent *dentry;
    struct stat sb; //struttura di supporto per verificare la dimensione del singolo file

    //check degli argomenti passati
    if(argc != 2){
        printf("Only the directory path needed");
        return 1;
    }


    //creazione, inizializzazione e gestione dei segnali
    sigset_t signalSet;
    
    //rimozione di tutti i segnali

    //variante
    /*if(sigfillset(&signalSet) || sigdelset(&signalSet, SIGINT | SIGUSR1) ||
            sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
        ErrExit("Something wrong I can smell it");
    }*/

    if(sigfillset(&signalSet) == -1){
        ErrExit("Failed to fill the signals");
    }
    //aggiunta dei segnali SIGINT e SIGUSR1 alla maschera
    if(sigdelset(&signalSet, SIGINT | SIGUSR1) == -1){
        ErrExit("Failed to set the signals");
    }
    if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
        ErrExit("Failed to set the signal mask");
    }

    //settaggio del sigHandler
    if(signal(SIGUSR1 | SIGINT, sigHandler) == SIG_ERR){
        ErrExit("Signal handler set failed");
    }
    //messo in attesa dei segnali desiderati
    while(1){
        pause();
        //settaggio maschera
        if(sigaddset(&signalSet, SIGINT | SIGUSR1) == -1){
            ErrExit("Failed to reset the signals");
        }
        //impostazione della directory
        if(chdir(argv[1]) == -1){
            ErrExit("Failed to switch directory");
        }

        //otteniamo la current working directory
        if(getcwd(currdir,sizeof(currdir)) == NULL){
            ErrExit("getcwd failed!");
        }
        
        printf("Ciao %s, ora inizio l’invio dei file contenuti in %s", getenv("USER"), currdir);
        DIR *dirp = opendir(currdir);

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
                char result[250];
                *result = strcat(currdir, dentry->d_name); //FORSE BISOGNA METTERE '/' DOPO IL PATH DELLA CARTELLA
                stat(result, &sb);
                if(sb.st_size <= 4096){
                    //il file è minore uguale a 4KB, bisogna SISTEMARE
                }
            }
            }

        }




    return 0;
}
