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
#include <errno.h>

#include "defines.h"
#include "err_exit.h"
#include "semaphore.h"
#include "shared_memory.h"

#define BUFFER_SZ 150
#define N_FILES 100

char currdir[BUFFER_SZ];
char *files[N_FILES]; //pathname file
int n_files = 0;
char *fifo1name = "/tmp/myfifo1";
char *fifo2name ="/tmp/myfifo2";
char *fifoDummy = "/tmp/myFifoDummy";

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
        int fdDummy = open(fifoDummy, O_RDONLY);
        int shmId;
        read(fdDummy, shmId, sizeof(shmId));
        close(fdDummy); //chiusura della FIFO

        //stessa roba con msgQ
        int fdsh = open(fifoDummy, O_RDONLY);
        int shmId; //sarebbe la msgQ TODO
        read(fdDummy, shmId, sizeof(shmId));
        close(fdDummy); //chiusura della FIFO

        //attende di ricevere messaggio da shared memory TODO tutto
        char *shmPointer = (char *) shmat(shmId, NULL, 0); //TODO perché abbiamo copiato funzione dal prof
        while(shmPointer != "confirmed");

        //creazione del semaforo che verrà usato dai figli
        int semForIPC=semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
        if(semForIPC == -1)
            errExit("semget failed");

        //inizializzazione del semaforo
        union semun arg;
        arg.val=n_files; //set del valore iniziale
        if(semctl(semForIPC, 0, SETALL, arg) == -1){
            errExit("semctl set failed!");
        }

        //definizione delle strutture che verranno usate per l'invio
        struct mymsg sendByFIFO1;
        struct mymsg sendByFIFO2;
        struct mymsg *sendByShMemory;
        struct mymsg sendByMsgQ;

        //attach della shared memory
        sendByShMemory = (struct mymsg*) get_shared_memory(shmId, 0);

        //lettura ID dei semafori generati dal server
        int semIdForFIFO1, semIdForFIFO2, semIdForShMemory, semIdForMsgQ;
        read(fdDummy, semIdForFIFO1, sizeof(semIdForFIFO1));
        read(fdDummy, semIdForFIFO2, sizeof(semIdForFIFO2));
        read(fdDummy, semIdForShMemory, sizeof(semIdForShMemory));
        read(fdDummy, semIdForMsgQ, sizeof(semIdForMsgQ));

        close(fdDummy);

        //settaggio dei singoli semafori
        union semun semFIFO1;
        union semun semFIFO2;
        union semun semShMemory;
        union semun semMsgQ;
        semFIFO1.val = 50;
        semFIFO2.val = 50;
        semShMemory.val = 50;
        semMsgQ.val = 50;
        if(semctl(semIdForFIFO1,0,SETALL, semFIFO1) == -1){
            errExit("semctl for semFIFO1 set failed!");
        }
        if(semctl(semIdForFIFO2,0,SETALL, semFIFO2) == -1){
            errExit("semctl for semFIFO2 set failed!");
        }
        if(semctl(semIdForShMemory,0,SETALL, semShMemory) == -1){
            errExit("semctl for semShMemory set failed!");
        }
        if(semctl(semIdForMsgQ,0,SETALL, semMsgQ) == -1){
            errExit("semctl for semMsgQ set failed!");
        }

        //generazione figli
        pid_t pid;
        for(int child = 0; child < n_files; child++){
            pid=fork();
            if(pid == -1)
                errExit("fork error");
            else if(pid == 0){
                struct stat fileStatistics;
                struct mymsg *dummyShM;
                char *buff;
                //salvataggio del pid del figlio
                sendByFIFO1.mtype = getpid();
                sendByFIFO2.mtype = getpid();
                sendByMsgQ.mtype = getpid();
                dummyShM->mtype = getpid();
                //salvataggio pathname del file
                sendByFIFO1.pathname = files[child];
                sendByFIFO2.pathname = files[child];
                sendByMsgQ.pathname = files[child];
                dummyShM->pathname = files[child];

                int fd = open(files[child], O_RDONLY, S_IRUSR); //apertura child-esimo file
                stat(files[child], &fileStatistics); //prendo statistiche file
                read(fd, buff, fileStatistics.st_size); //leggo il file

                divideString(buff,sendByFIFO1.portion,sendByFIFO2.portion,sendByMsgQ.portion,dummyShM->portion); //dividiamo il file e lo salviamo nelle stringhe

                //blocco il figlio
                semOp(semForIPC, (unsigned short)0, -1); //TODO: Da verificare!
                semOp(semForIPC, (unsigned short)0, 0); //Rimane fermo fin quando tutti non sono 0.
                //iniziano inviare //TODO forse non va bene che inviano uno alla volta
                semOp(semIdForFIFO1, 0, -1);
                int fdFIFO1 = open(fifo1name, S_IWUSR);
                if(write(fdFIFO1, sendByFIFO1, sizeof(sendByFIFO1)) == -1){
                    errExit("Client, failed to write on FIFO1");
                }

                semOp(semIdForFIFO2, 0, -1);
                int fdFIFO2 = open(fifo2name, S_IWUSR);
                if(write(fdFIFO2, sendByFIFO2, sizeof(sendByFIFO2)) == -1){
                    errExit("Client, failed to write on FIFO2");
                }

                semOp(semIdForShMemory, 0, -1);
                sendByShMemory[child] = *dummyShM; //TODO da verificare se funziona esattamente così
                /* TODO creare variabile inizializzata a 0, man mano che si scrive
                 * viene incrementata di 1 e per quando deve ripartire da 0 */
                /* 1 2 3 4 5
                 * 0 2 3 4 5
                 * 6 2 3 4 5 */


                
            }
            else{
                //codice padre client_0
                //attesa msg queue
            }
        }
    }

    return 0;
}
