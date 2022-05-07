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
#define N_FILES 100

char currdir[BUFFER_SZ];
char *files[100]; //nomi file
int n_files=0;
char *fifo1name="/tmp/myfifo1";


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
        
        search(files, currdir, n_files);
        //invio n° file
        int fd1=open(fifo1name,O_WRONLY);

        write(fd1,n_files,sizeof(n_files));
        
        //attende di ricevere messaggio da shared memory

        //generazione figli
        pid_t pid;
        for(int i=0;i<n_files;i++){
            pid=fork();
            if(pid==-1)
                ErrExit("fork error");
            else if(pid == 0){
                //figlio fa cose
                
            }
        }

        if(pid==-1)
            ErrExit("fork error!");
        if(pid==0)
            //figlio che fa cose...
	
        }
		



    return 0;
}
