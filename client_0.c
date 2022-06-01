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
#include <sys/msg.h>
#include <sys/wait.h>

#include "defines.h"
#include "err_exit.h"
#include "semaphore.h"
#include "shared_memory.h"

#define BUFFER_SZ 150
#define N_FILES 100 //numero massimo di file da elaborato
#define MAX_PATH 256

char currdir[BUFFER_SZ];
char files[N_FILES][MAX_PATH]; //array contenente tutti i pathname
int n_files = 0; //TODO perché è qui? non serve
char *fifo1name = "/tmp/myfifo1";
char *fifo2name ="/tmp/myfifo2";
//char *fifoDummy = "/tmp/myfifodummy";

void sigHandler(int sig) {
    if(sig == SIGUSR1){
        printf("Terminazione del processo Client_0.\n");
        if(kill(getpid(), SIGTERM) == -1){
            errExit("Ops, il Client_0 è sopravvissuto");
        }
    }
    else if(sig == SIGINT) {
        printf("Avvio di Client_0...\n");
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
        printf("<Client_0> Devi passarmi il path della directory!\n");
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
        errExit("<Client_0> Non sono riuscito a riempire il set di segnali\n");
    }
    //rimozione dei segnali SIGINT e SIGUSR1 alla maschera
    if(sigdelset(&signalSet, SIGINT) == -1){
        errExit("<Client_0> Non sono riuscito a settare i segnali\n");
    }
    if(sigdelset(&signalSet, SIGUSR1) == -1){
        errExit("<Client_0> Non sono riuscito a settare i segnali\n");
    }
    if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
        errExit("<Client_0> Non sono riuscito a settare la maschera dei segnali\n");
    }

    //settaggio dei segnali al sigHandler
    if(signal(SIGUSR1, sigHandler) == SIG_ERR){
        errExit("<Client_0> Non sono riuscito a settare il signal handler\n");
    }
    if(signal(SIGINT, sigHandler) == SIG_ERR){
        errExit("<Client_0> Non sono riuscito a settare il signal handler\n");
    }
    //in attesa dei segnali desiderati
    while(1){
        printf("<Client_0> In attesa di CTRL+C...\n");
        pause();

        //aperture FIFO + invio PID Client_0
        int fdFIFO1 = open(fifo1name, O_WRONLY);
        if(fdFIFO1 == -1)
            errExit("<Client_0> Errore con apertura fifo1\n");
        int fdFIFO2 = open(fifo2name,O_WRONLY);
        if(fdFIFO2 == -1)
            errExit("<Client_0> Errore con apertura fifo2\n");
        printf("<Client_0> Ho aperto FIFO1 e FIFO2\n");
        pid_t pidclient = getpid();
        write(fdFIFO1, &pidclient, sizeof(pid_t));
        printf("<Client_0> Invio il mio PID: %d\n", pidclient);

        //settaggio maschera
        if(sigaddset(&signalSet, SIGINT) == -1){
            errExit("<Client_0> Non sono riuscito a riaggiungere i segnali al set\n");
        }
        if(sigaddset(&signalSet, SIGUSR1) == -1){
            errExit("<Client_0> Non sono riuscito a riaggiungere i segnali al set\n");
        }
        if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
            errExit("<Client_0> Non sono riuscito a resettare la maschera dei segnali\n");
        }
        printf("<Client_0> Ho risettato i segnali\n");
        //impostazione della directory
        if(chdir(argv[1]) == -1){
            errExit("<Client_0> Non sono riuscito a cambiare directory\n");
        }
        printf("<Client_0> Ho cambiato directory, mi sono spostato in %s\n", argv[1]);

        //otteniamo la current working directory
        if(getcwd(currdir,sizeof(currdir)) == NULL){
            errExit("<Client_0> Non sono riuscito a ottenre la directory attuale\n");
        }
        
        printf("<Client_0> Ciao %s, ora inizio l’invio dei file contenuti in %s\n", getenv("USER"), currdir);

        printf("<Client_0> Inizio a cercare\n");
        printf("currdir: %s\n",currdir);
        n_files = search(files, argv[1], 0); //TODO n_files non viene aggiornato
        printf("<Client_0> Ho finito, ho trovato %d files\n", n_files);

        //invio del numero di files al server
        write(fdFIFO1, &n_files, sizeof(int));

        //chiusura momentanea per ricezione ID IPC
        close(fdFIFO1);
        printf("<Client_0> Ho chiuso la fifo1 in scrittura\n");
        fdFIFO1 = open(fifo1name,O_RDONLY);
        printf("<Client_0> Ho aperto la fifo1 in lettura\n");

        int shmId;
        printf("<Client_0> Sto per ricevere l'ID della shared memory\n");
        read(fdFIFO1, &shmId, sizeof(shmId));
        printf("smhid: %d\n", shmId);

        //stessa roba con msgQ
        int msgQId; //sarebbe la msgQ
        printf("<Client_0> Sto per ricevere l'ID della message queue\n");
        read(fdFIFO1, &msgQId, sizeof(msgQId));
        printf("msgqid: %d\n", msgQId);

        //lettura ID dei semafori generati dal server
        int semIdForIPC;
        read(fdFIFO1, &semIdForIPC, sizeof(semIdForIPC));
        if(close(fdFIFO1) == -1)
            errExit("<Client_0> Non son riuscito a chiudere la fifo1 in lettura\n");

        printf("<Client_0> Ho chiuso la fifo1 in lettura\n");
        printf("<Client_0> Riapro la fifo1 in scrittura\n");
        sleep(5); //facciamo attendere 10 secondi per sicurezza
        fdFIFO1 = open(fifo1name, O_WRONLY);
        printf("<Client_0> Ho riaperto la fifo1 in scrittura\n");
        //definizione delle strutture che verranno usate per l'invio
        struct mymsg sendByFIFO1;
        struct mymsg sendByFIFO2;
        struct mymsg *sendByShMemory;
        struct mymsg sendByMsgQ;
        struct mymsg dummyShM; //servirà per la shared memory con i client figli

        //attach della shared memory
        printf("<Client_0> Sto per fare la prima attach\n");
        sendByShMemory=(struct mymsg*) get_shared_memory(shmId, 0);
        /* variabili per scorrere lungo la shared memory
         * cursor è il puntatore al blocco attuale
         * maxCursor è la dimensione massima della shared memory
         */
        int cursor = 0, maxCursor = 50; //cambiato da sizeof(struct)*50

        //attende di ricevere messaggio conferma ricezione numero file tramite shared memory
        while(sendByShMemory[0].mtype != 1); //rimane bloccato fintanto che non riceve conferma
        printf("<Client_0> ricevuta conferma ricezione numero file dal server\n");

        //creazione del semaforo per partenza sincrona dei figli
        int semForChild = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
        if(semForChild == -1)
            errExit("<Client_0> Non sono riuscito a creare il set di semafori\n");

        //inizializzazione del semaforo
        unsigned short monoArray[1];
        monoArray[0]=n_files;
        union semun argSemForChild;
        argSemForChild.array = monoArray; //set del valore iniziale
        printf("<Client_0> Sto per settare il semaforo di attesa collettiva dei figli\n");
        if(semctl(semForChild, 0, SETALL, argSemForChild) == -1){
            errExit("<Client_0> Non sono riuscito a settare i semafori per l'attesa collettiva dei figli\n");
        }
        if(semctl(semForChild,0,GETALL,argSemForChild)==-1)
            errExit("Errore client_0");
        printf("valore semforchild: %d",argSemForChild.array[0]);

        //inizializzo la shared memory di mtype = 0 per agevolare lettura da server
        for(int i = 0; i < 50; i++){
            sendByShMemory[i].mtype = 0;
        }
        printf("<Client_0> Ho inizializzato la shared memory\n");

        //mutex per accesso al valore del semaforo per la singola IPC e gestione del cursore della shared memory
        int mutex = semget(IPC_PRIVATE,5,IPC_CREAT|S_IRUSR|S_IWUSR);
        unsigned short values[] = {1,1,1,1,1};
        union semun argMutex;
        argMutex.array = values;

        if(semctl(mutex, 0, SETALL, argMutex) == -1)
            errExit("<Client_0> Non sono riuscito a settare il semaforo mutex\n");
        printf("<Client_0> Ho generato e settato il semaforo mutex\n");

        //generazione figli
        pid_t pid;
        for(int child = 0; child < n_files; child++){
            pid = fork();
            if(pid == -1)
                errExit("<Client_0> Non sono riuscito a fare la fork\n");
            else if(pid == 0){
                //array di supporto per il singolo figlio per verificare se ha già inviato tramite una IPC
                printf("Ciao! Sono il figlio numero %d e sto per iniziare a lavorare!\n", child+1);
                int checkinvio[]={0,0,0,0};
                struct stat fileStatistics;
                char buff[4100];
                int pid = getpid();
                //salvataggio del pid del figlio
                fflush(stdout);
                printf("<Client_%d> Sto inizializzando le strutture con il mio pid\n", pid);
                sendByFIFO1.pid = pid;
                sendByFIFO2.pid = pid;
                sendByMsgQ.pid = pid;
                dummyShM.pid = pid;
                //salvataggio pathname del file
                printf("<Client_%d> Sto inizializzando le strutture con il pathname assegnatomi: %s\n", pid, files[child]);
                strcpy(sendByFIFO1.pathname, files[child]);
                strcpy(sendByFIFO2.pathname, files[child]);
                strcpy(sendByMsgQ.pathname, files[child]);
                strcpy(dummyShM.pathname, files[child]);
                /* lasciamo così momentaneamente
                sendByFIFO1->pathname = files[child];
                sendByFIFO2->pathname = files[child];
                sendByMsgQ.pathname = files[child];
                dummyShM->pathname = files[child];
                */

                printf("<Client_%d> Sto per aprire il file %s\n", pid, files[child]);
                int fd = open(files[child], O_RDONLY, S_IRUSR); //apertura child-esimo file
                if(fd==-1)
                    errExit("<Client_figlio> Errore client open");
                printf("<Client_%d> Ho aperto il file!\n", pid);
                stat(files[child], &fileStatistics); //prendo statistiche file
                read(fd, buff, fileStatistics.st_size); //leggo il file
                printf("<Client_%d> Sto per dividere il file\n", pid);
                divideString(buff,sendByFIFO1.portion,sendByFIFO2.portion,sendByMsgQ.portion,dummyShM.portion); //dividiamo il file e lo salviamo nelle stringhe
                printf("<Client_%d> Ho diviso il file!\n", pid);
                //blocco il figlio
                printf("sto per semaforo");
                semOp(semForChild, (unsigned short)1, -1); //TODO: Da verificare!
                semOp(semForChild, (unsigned short)1, 0); //Rimane fermo fin quando tutti non sono 0.
                //iniziano inviare
                printf("post semaforo");

                do{
                    //INIZIO INVIO
                    printf("<Client_%d> Sto per iniziare ad inviare\n", pid);
                    //scrittura in FIFO1
                    if(checkinvio[0]!=1){
                        printf("semaforo riga 281");
                        semOp(mutex,(unsigned short)0,-1);
                        printf("<Client_%d> Sto per verificare il valore del semaforo della fifo1\n", pid);
                        int semFIFO1value = semctl(semIdForIPC, 1, GETVAL, 0); //recupero il valore del semaforo per verificare se l'IPC è piena o no
                        if(semFIFO1value == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della FIFO1\n");
                        else if(semFIFO1value > 0){
                            printf("<Client_%d> Ok sto per entrare nella coda\n", pid);
                            printf("semaforo riga 289");
                            semOp(semIdForIPC,(unsigned short)0,-1);//mi prenoto il posto nell'IPC
                            semOp(mutex,(unsigned short)0,1); //lascio accedere alla mutex al prossimo client
                            printf("<Client_%d> Sto per fare l'invio...\n",pid);
                            if(write(fdFIFO1, &sendByFIFO1, sizeof(sendByFIFO1)) == -1){
                                errExit("<Client_0> Non sono riuscito a scrivere nella FIFO1\n");
                            }
                            printf("<Client_%d> ho fatto l'invio\n",pid);
                            checkinvio[0]=1;
                            close(fdFIFO1);
                        }
                    }

                    //scrittura in FIFO2
                    if(checkinvio[1]!=1){
                        printf("semaforo riga 304");
                        semOp(mutex,(unsigned short)1,-1);
                        int semFIFO2value = semctl(semIdForIPC, 2, GETVAL, 0);
                        if(semFIFO2value == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della FIFO2\n");
                        else if(semFIFO2value > 0){
                            printf("semaforo riga 310");
                            semOp(semIdForIPC,(unsigned short)1,-1);
                            semOp(mutex,(unsigned short)1,1);
                            if(write(fdFIFO2, &sendByFIFO2, sizeof(sendByFIFO2)) == -1){
                                errExit("<Client_0> Non sono riuscito a scrivere nella FIFO2\n");
                            }
                            checkinvio[1]=1;
                            close(fdFIFO2);
                        }
                    }

                    //scrittura in Shared Memory
                    if(checkinvio[2]!=1) {
                        printf("semaforo riga 323");
                        semOp(mutex, (unsigned short)2, -1);
                        int semShMvalue = semctl(semIdForIPC, 3, GETVAL, 0);
                        if (semShMvalue == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della shared memory\n");
                        else if (semShMvalue > 0) {
                            printf("semaforo riga 329");
                            semOp(semIdForIPC, (unsigned short)2, -1);
                            semOp(mutex, (unsigned short)2, 1);
                            semOp(mutex, (unsigned short)4, -1);

                            memcpy(&sendByShMemory[cursor].pid,&dummyShM.pid, sizeof(int));
                            //sendByShMemory[cursor].pid = dummyShM.pid;//copio il pid di dummy nella shared memory
                            strcpy(sendByShMemory[cursor].pathname, dummyShM.pathname);
                            strcpy(sendByShMemory[cursor].portion, dummyShM.portion);
                            cursor++;

                            if (cursor == maxCursor)
                                cursor = 0;
                            printf("semaforo riga 342");
                            semOp(mutex, (unsigned short)4, 1);
                            checkinvio[2] = 1;
                            free_shared_memory(sendByShMemory);
                        }
                    }

                    //Msg Queue
                    if(checkinvio[3]!=1){
                        printf("semaforo riga 350");
                        semOp(mutex, (unsigned short)3,-1);
                        int semMsgQvalue = semctl(semIdForIPC, 4, GETVAL, 0);
                        if(semMsgQvalue == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della message queue\n");
                        else if(semMsgQvalue > 0){
                            printf("semaforo riga 356");
                            semOp(semIdForIPC,(unsigned short)3,-1);
                            semOp(mutex, (unsigned short)3,1);
                            int sizeOfMsg = sizeof(struct mymsg) - sizeof(long);
                            if(msgsnd(msgQId,&sendByMsgQ,sizeOfMsg,0) == -1)
                                errExit("<Client_0> Non sono riuscito a inviare un messaggio nella message queue\n");
                            checkinvio[3] = 1;
                        }
                    }

                }while(checkinvio[0] != 1 && checkinvio[1] != 1 && checkinvio[2] != 1 && checkinvio[3] != 1);

                close(fd);
                kill(getpid(),SIGTERM);
                
            }

        }
        /*Client_0 si mette in attesa di conferma fine lavoro da
                 * server la conferma sarà un 1 in mtype
                 */
        struct mymsg result;
        int flag = 1, sizeOfResult = sizeof(result) - sizeof(long);
        while(flag){
            if(msgrcv(msgQId, &result, sizeOfResult, 1, 0) != -1)
                flag = 0;
        }

        //rimozione dei semafori semForChild e mutex
        if(semctl(semForChild, 1, IPC_RMID, 0) == -1)
            errExit("<Client_0> Non sono riuscito a eliminare il semaforo semForChild\n");
        if(semctl(mutex, 0, IPC_RMID, 0) == -1)
            errExit("<Client_0> Non sono riuscito a eliminare il semaforo mutex\n");

        //riempio la signalSet con tutti i segnali
        if(sigfillset(&signalSet) == -1){
            errExit("\"<Client_0> Non sono riuscito a riempire il set di segnali\\n\"");
        }
        //aggiunta dei segnali SIGINT e SIGUSR1 alla maschera
        if(sigdelset(&signalSet, SIGINT | SIGUSR1) == -1){
            errExit("\"<Client_0> Non sono riuscito a togliere SIGINT e SIGUSR1 dal set di segnali\\n\"");
        }
        if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
            errExit("<Client_0> Non sono riuscito a settare la maschera dei segnali\n");
        }

    }

    return 0;
}
