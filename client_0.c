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
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "defines.h"
#include "err_exit.h"
#include "semaphore.h"

#define BUFFER_SZ 150
#define N_FILES 100

char currdir[BUFFER_SZ];
char *files[N_FILES]; //pathname file
int n_files = 0;
char *fifo1name = "/tmp/myfifo1";
char *fifoShmId = "/tmp/myFifoForId";
int attesa=0;

void sigHandler(int sig) {
    if(sig == SIGUSR1){
        printf("Termination of Client_0 process.\n");
        if(kill(getpid(), SIGTERM) == -1){
            errExit("Failed to terminate client_0");
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

    //controllo gli argomenti passati
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
        errExit("Something wrong I can smell it");
    }*/

    //riempio la signalSet con tutti i segnali
    if(sigfillset(&signalSet) == -1){
        errExit("Failed to fill the signals");
    }
    //aggiunta dei segnali SIGINT e SIGUSR1 alla maschera
    if(sigdelset(&signalSet, SIGINT | SIGUSR1) == -1){
        errExit("Failed to set the signals");
    }
    if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
        errExit("Failed to set the signal mask");
    }

    //settaggio dei segnali al sigHandler
    if(signal(SIGUSR1 | SIGINT, sigHandler) == SIG_ERR){
        errExit("Signal handler set failed");
    }
    //in attesa dei segnali desiderati
    while(1){
        pause();
        //settaggio maschera
        if(sigaddset(&signalSet, SIGINT | SIGUSR1) == -1){
            errExit("Failed to reset the signals");
        }
        //impostazione della directory
        if(chdir(argv[1]) == -1){
            errExit("Failed to switch directory");
        }

        //otteniamo la current working directory
        if(getcwd(currdir,sizeof(currdir)) == NULL){
            errExit("getcwd failed!");
        }
        
        printf("Ciao %s, ora inizio l’invio dei file contenuti in %s", getenv("USER"), currdir);
        
        search(files, currdir, n_files);
        //invio n° file
        int fd1 = open(fifo1name,O_WRONLY);

        write(fd1, n_files, sizeof(n_files));

        //server crea una fifo ad hoc per inviare l'id del segmento di memoria da cui successivamente
        //il client otterrà l'ok dal server
        int fdId = open(fifoShmId, O_RDONLY);
        int shmId;
        read(fdId, shmId, sizeof(shmId));
        close(fdId); //chiusura della FIFO

        //attende di ricevere messaggio da shared memory
        char *shmPointer = (char *) shmat(shmId, NULL, 0);
        while(shmPointer != "confirmed");

        //creazione del semaforo che verrà usato dai figli
        int semid=semget(IPC_PRIVATE,1,S_IRUSR|S_IWUSR);
        if(semid==-1)
            errExit("semget failed");

        //inizializzazione del semaforo
        union semun arg;
        arg.val=n_files; //set del valore iniziale
        if(semctl(semid,0,SETALL,arg)==-1){
            errExit("semctl set failed!");
        }

        //generazione figli
        pid_t pid;
        for(int child = 0; child < n_files; child++){
            pid=fork();
            if(pid == -1)
                errExit("fork error");
            else if(pid == 0){
                struct stat sb;
                char *buff;
                char *sendByFIFO1;
                char *sendByFIFO2;
                char *sendByMsgQ; //variabile in cui salvare per msgQ
                char *sendByShM; //variabile in cui salvare per shm
                int fd, counter = 0, incremento = 0, totalByte = 0, porzione = 0, bloccoByte = 0;

                fd = open(files[child], O_RDONLY, S_IRUSR); //apertura child-esimo file
                stat(files[child], &sb); //prendo statistiche file
                read(fd, buff, sb.st_size); //leggo il file

                divideString(buff,sendByFIFO1,sendByFIFO2,sendByMsgQ,sendByShM); //dividiamo il file e lo salviamo nelle stringhe


                //blocco il figlio
                attesa++; //controllo attesa figli
                semOp(semid, (unsigned short)0, -1); //TODO: Da verificare!
                semOp(semid, (unsigned short)0, 0); //Rimane fermo fin quando tutti non sono 0.
                //iniziano inviare

                
            }else{
                //codice padre client_0
                //attesa msg queue

            }
        }

        if(pid == -1)
            errExit("fork error!");
        if(pid == 0)
            //figlio che fa cose...
	
    }

    return 0;
}
