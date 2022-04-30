/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

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

size_t append2Path(char *directory){
    size_t lastPathEnd=strlen(currdir);
    strcat(strcat(&currdir[lastPathEnd],"/"),directory);
    return lastPathEnd;
}

int checkFileName(char *filename,char *string){
    //controllo nome file

        //1: ok
        //0: no
    }

}

int main(int argc, char * argv[]) {

    //creazione, inizializzazione e gestione dei segnali
    sigset_t signalSet;
    char *user;
    
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

        user=getenv("USER");
        if(getcwd(currdir,sizeof(currdir))==NULL){
            ErrExit("getcwd failed!");
        }
        
        printf("Ciao %s, ora inizio l’invio dei file contenuti in %s",user,currdir);
        DIR *dirp=opendir(currdir);
        
        struct dirent *dentry;

        while((dentry=readdir(dirp))!=NULL){
            if(strcmp(dentry->d_name,".")==0||strcmp(dentry->d_name,".."==0)){
                continue;
            }
            if(dentry->d_type==DT_REG){
                size_t lastPath=append2Path(dentry->d_name); 

                int match=checkFileName(dentry->d_name, dentry->d_name + "/sendme_");

                if(match==1) //true

            }



        }




    return 0;
}
