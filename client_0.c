/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/msg.h>

#include "defines.h"
#include "err_exit.h"
#include "semaphore.h"
#include "shared_memory.h"

char currdir[BUFFER_SZ];
char files[N_FILES][MAX_PATH]; //array contenente tutti i pathname
char *fifo1name = "/tmp/myfifo1";
char *fifo2name ="/tmp/myfifo2";
int n_files;
int semForChild;
int mutex;
int semCursor;

void sigHandler(int sig) {
    if(sig == SIGUSR1){
        printf("Terminazione del processo Client_0.\n");
        if(semctl(semForChild,0,IPC_RMID,0) == -1)
            errExit("Errore cancellazione semForChild");
        if(semctl(mutex,0,IPC_RMID,0) == -1)
            errExit("Errore cancellazione mutex");
        if(semctl(semCursor,0,IPC_RMID,0) == -1)
            errExit("Errore cancellazione semCursor");

        if(kill(getpid(), SIGTERM) == -1){
            errExit("Ops, il Client_0 è sopravvissuto");
        }
    }
    else if(sig == SIGINT) {
        printf("Avvio di Client_0...\n");
    }
}

int main(int argc, char * argv[]) {

    //controllo gli argomenti passati
    if(argc != 2){
        printf("<Client_0> Devi passarmi il path della directory!\n");
        return 1;
    }

    //creazione, inizializzazione e gestione dei segnali
    sigset_t signalSet;
    
    //rimozione di tutti i segnali

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

    while(1){

        printf("<Client_0> In attesa di CTRL+C...\n");
        n_files = 0; //reset di files
        pause();
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
        n_files = 0;
        n_files = search(files, argv[1], 0);
        if(n_files == 0){
            printf("<Client_0> Ho trovato 0 files, chiudo\n");
            exit(1);
        }

        if(n_files == 100){
            printf("<Client_0> Ho trovato 100 file, mi fermo qui e inizio a lavorare con loro\n");
        }
        printf("<Client_0> Ho finito, ho trovato %d files\n", n_files);

        //invio del numero di files al server

        if(write(fdFIFO1, &n_files, sizeof(int)) == -1)
            errExit("<Client_0> Errore scrittura fifo1");

        //chiusura momentanea per ricezione ID IPC TODO controllo close, open,...
        if(close(fdFIFO1)==-1)
            errExit("<Client_0> Errore chiusura fifo1");
        printf("<Client_0> Ho chiuso la fifo1 in scrittura\n");
        fdFIFO1 = open(fifo1name,O_RDONLY);
        if(fdFIFO1==-1)
            errExit("<Client_0> Errore apertura fifo1");
        printf("<Client_0> Ho aperto la fifo1 in lettura\n");

        int shmId;
        printf("<Client_0> Sto per ricevere l'ID della shared memory\n");
        if(read(fdFIFO1, &shmId, sizeof(shmId))==-1)
            errExit("<Client_0> Errore lettura idShm fifo1");

        //stessa roba con msgQ
        int msgQId; //sarebbe la msgQ
        printf("<Client_0> Sto per ricevere l'ID della message queue\n");
        if(read(fdFIFO1, &msgQId, sizeof(int)) == -1)
            errExit("<Client_0> Errore lettura idMsgQ fifo1");

        //lettura ID dei semafori generati dal server
        int semIdForIPC;
        if(read(fdFIFO1, &semIdForIPC, sizeof(semIdForIPC)) == -1)
            errExit("<Client_0> Errore lettura fifo1");
        if(close(fdFIFO1) == -1)
            errExit("<Client_0> Non son riuscito a chiudere la fifo1 in lettura\n");

        printf("<Client_0> Ho chiuso la fifo1 in lettura\n");
        printf("<Client_0> Riapro la fifo1 in scrittura\n");
        sleep(5); //facciamo attendere 10 secondi per sicurezza
        fdFIFO1 = open(fifo1name, O_WRONLY);
        if(fdFIFO1 == -1)
            errExit("<Client_0> Non son riuscito ad aprire la fifo1\n");
        printf("<Client_0> Ho riaperto la fifo1 in scrittura\n");
        //definizione delle strutture che verranno usate per l'invio
        struct mymsg sendByFIFO1;
        struct mymsg sendByFIFO2;
        struct mymsg *sendByShMemory;
        struct mymsg sendByMsgQ;
        struct mymsg dummyShM; //servirà per la shared memory con i client figli

        //attach della shared memory
        printf("<Client_0> Sto per fare la prima attach\n");
        sendByShMemory = (struct mymsg*) get_shared_memory(shmId, 0);
        /* variabili per scorrere lungo la shared memory
         * cursor è il puntatore al blocco attuale
         */
        int cursor = 0;

        //attende di ricevere messaggio conferma ricezione numero file tramite shared memory
        while(sendByShMemory[0].mtype != 1); //rimane bloccato fintanto che non riceve conferma
        printf("<Client_0> Ricevuta conferma ricezione numero file dal server\n");

        //creazione del semaforo per partenza sincrona dei figli
        semForChild = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
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

        //inizializzo la shared memory di mtype = 2 per agevolare lettura da server
        for(int i = 0; i < 50; i++){
            sendByShMemory[i].mtype = 2;
        }
        printf("<Client_0> Ho inizializzato la shared memory\n");

        //mutex per accesso al valore del semaforo per la singola IPC e gestione del cursore della shared memory
        mutex = semget(IPC_PRIVATE,5,IPC_CREAT|S_IRUSR|S_IWUSR);
        /* 0 -> mutex per lettura valore semaforo FIFO1
         * 1 -> mutex per lettura valore semaforo FIFO2
         * 2 -> mutex per lettura valore semaforo Shared Memory
         * 3 -> mutex per lettura valore semaforo MsgQ
         * 4 -> mutex per poter entrare a scrivere in Shared Memory
         */
        unsigned short values[] = {1,1,1,1,1};
        union semun argMutex;
        argMutex.array = values;

        if(semctl(mutex, 0, SETALL, argMutex) == -1)
            errExit("<Client_0> Non sono riuscito a settare il semaforo mutex\n");
        printf("<Client_0> Ho generato e settato il semaforo mutex\n");

        //creazione semaforo che farà ci aiuterà con il cursore della shared memory
        semCursor = semget(IPC_PRIVATE, 1, IPC_CREAT|S_IRUSR|S_IWUSR);
        unsigned short cursorValue[1] = {0};
        union semun argCursor;
        argCursor.array = cursorValue;

        if(semctl(semCursor, 0, SETALL, argCursor) == -1)
            errExit("<Client_0> Non sono riuscito a settare il semaforo per la gestione del cursore della shared memory\n");

        //generazione figli
        pid_t pid;
        for(int child = 0; child < n_files; child++){
            pid = fork();
            if(pid == -1)
                errExit("<Client_0> Non sono riuscito a fare la fork\n");
            else if(pid == 0){
                //array di supporto per il singolo figlio per verificare se ha già inviato tramite una IPC
                int checkinvio[]={0,0,0,0};
                struct stat fileStatistics;
                char buff[4100] = "";
                int pid = getpid();
                printf("Ciao! Sono il figlio numero %d con PID %d e sto per iniziare a lavorare!\n", child+1, pid);
                //salvataggio del pid del figlio
                fflush(stdout);
                printf("<Client_%d> Sto inizializzando le strutture con il mio pid\n", pid);
                sendByFIFO1.pid = pid;
                sendByFIFO2.pid = pid;
                sendByMsgQ.pid = pid;
                dummyShM.pid = pid;

                //inizializzo mtype
                sendByMsgQ.mtype = 2;
                //salvataggio pathname del file
                printf("<Client_%d> Sto inizializzando le strutture con il pathname assegnatomi: %s\n", pid, files[child]);
                strcpy(sendByFIFO1.pathname, files[child]);
                strcpy(sendByFIFO2.pathname, files[child]);
                strcpy(sendByMsgQ.pathname, files[child]);
                strcpy(dummyShM.pathname, files[child]);

                strcpy(sendByFIFO1.portion,"");
                strcpy(sendByFIFO2.portion,"");
                strcpy(sendByMsgQ.portion,"");
                strcpy(dummyShM.portion,"");


                printf("<Client_%d> Sto per aprire il file %s\n", pid, files[child]);
                int fd = open(files[child], O_RDONLY, S_IRUSR); //apertura child-esimo file
                if(fd==-1)
                    errExit("<Client_figlio> Errore client open");
                printf("<Client_%d> Ho aperto il file!\n", pid);
                if(fstat(fd, &fileStatistics) == -1) //prendo statistiche file
                    errExit("<Client> Non son riuscito a fare la fstat\n");
                read(fd, buff, fileStatistics.st_size); //leggo il file
                printf("<Client_%d> Sto per dividere il file\n", pid);
                divideString(buff,sendByFIFO1.portion,sendByFIFO2.portion,sendByMsgQ.portion,dummyShM.portion); //dividiamo il file e lo salviamo nelle stringhe
                printf("<Client_%d> Ho diviso il file!\nEcco le info che ho ottenuto:\npathname: %s\nper FIFO1: %s\nper FIFO2: %s\nper MsgQ: %s\nper ShM: %s\n", pid, dummyShM.pathname, sendByFIFO1.portion, sendByFIFO2.portion, sendByMsgQ.portion, dummyShM.portion);
                //blocco il figlio
                semOp(semForChild, (unsigned short)0, -1);
                semOp(semForChild, (unsigned short)0, 0); //Rimane fermo fin quando tutti non sono 0.
                //iniziano inviare
                do{

                    //scrittura in FIFO1
                    if(checkinvio[0] == 0){
                        semOp(mutex,(unsigned short)0,-1);
                        int semFIFO1value = semctl(semIdForIPC, 0, GETVAL, 0); //recupero il valore del semaforo per verificare se l'IPC è piena o no
                        if(semFIFO1value == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della FIFO1\n");
                        else if(semFIFO1value > 0){
                            semOp(semIdForIPC,(unsigned short)0,-1);//mi prenoto il posto nell'IPC
                            semOp(mutex,(unsigned short)0,1); //lascio accedere alla mutex il prossimo client
                            if(write(fdFIFO1, &sendByFIFO1, sizeof(sendByFIFO1)) == -1){
                                errExit("<Client_0> Non sono riuscito a scrivere nella FIFO1\n");
                            }
                            printf("<Client_%d> Ho fatto l'invio in FIFO1\n",pid);
                            checkinvio[0]=1;
                            close(fdFIFO1);
                        }
                        else{
                            semOp(mutex, (unsigned short)0, 1);
                        }
                    }

                    //scrittura in FIFO2
                    if(checkinvio[1] == 0){
                        semOp(mutex,(unsigned short)1,-1);
                        int semFIFO2value = semctl(semIdForIPC, 1, GETVAL, 0);
                        if(semFIFO2value == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della FIFO2\n");
                        else if(semFIFO2value > 0){
                            semOp(semIdForIPC,(unsigned short)1,-1);
                            semOp(mutex,(unsigned short)1,1);
                            if(write(fdFIFO2, &sendByFIFO2, sizeof(sendByFIFO2)) == -1){
                                errExit("<Client_0> Non sono riuscito a scrivere nella FIFO2\n");
                            }
                            printf("<Client_%d> Ho fatto l'invio in FIFO2\n",pid);
                            checkinvio[1]=1;
                            close(fdFIFO2);
                        }
                        else{
                            semOp(mutex, (unsigned short)1, 1);
                        }
                    }

                    //scrittura in Shared Memory
                    if(checkinvio[2] == 0) {
                        semOp(mutex, (unsigned short)2, -1); //per il valore di semidforipc
                        int semShMvalue = semctl(semIdForIPC, 2, GETVAL, 0);
                        if (semShMvalue == -1)
                            errExit("<Client> Non sono riuscito a recuperare il valore del semaforo della shared memory\n");
                        else if (semShMvalue > 0) {
                            semOp(semIdForIPC, (unsigned short)2, -1);
                            semOp(mutex, (unsigned short)2, 1);
                            semOp(mutex, (unsigned short)4, -1);//per accedere al cursore

                            /* Usiamo un semaforo per la gestione del cursore per scrivere sulla shared memory
                             * ogni client che riuscirà a entrare per scrivere sulla shared memory andrà
                             * a recuperare il valore del semaforo per il cursore (semCursor), controllerà
                             * che non sia già al valore 50 perché in quel caso uscirebbe dall'area dedicata
                             * alla shared memory, in tal caso lo resetta. Dopo che ha scritto farà una semOp
                             * per poter condividere con gli altri Client tale valore
                             */
                            cursor = semctl(semCursor, 0, GETVAL, 0);
                            if(cursor == -1)
                                errExit("<Client> Non sono riuscito a recuperare il valore del semaforo del cursore\n");
                            if(cursor == 50){
                                argCursor.array[0] = 0;
                                if(semctl(semCursor, 0, SETALL, argCursor) == -1)
                                    errExit("<Client> Non sono riuscito a resettare il valore del semaforo per il cursore\n");
                                cursor = 0;
                            }

                            memcpy(&sendByShMemory[cursor].pid,&dummyShM.pid, sizeof(int));

                            strcpy(sendByShMemory[cursor].pathname, dummyShM.pathname);
                            strcpy(sendByShMemory[cursor].portion, dummyShM.portion);
                            sendByShMemory[cursor].mtype = 0;

                            semOp(semCursor, (unsigned short)0, 1); //semaforo per gestire il cursore
                            semOp(mutex, (unsigned short)4, 1);
                            printf("<Client_%d> Ho fatto l'invio in shared memory\n",pid);
                            checkinvio[2] = 1;
                            free_shared_memory(sendByShMemory);
                        }
                        else{
                            semOp(mutex, (unsigned short)2, 1);
                        }
                    }

                    //Msg Queue
                    if(checkinvio[3] == 0){
                        semOp(mutex, (unsigned short)3,-1);
                        int semMsgQvalue = semctl(semIdForIPC, 3, GETVAL, 0);
                        if(semMsgQvalue == -1)
                            errExit("<Client_0> Non sono riuscito a recuperare il valore del semaforo della message queue\n");
                        else if(semMsgQvalue > 0){
                            semOp(semIdForIPC,(unsigned short)3,-1);
                            semOp(mutex, (unsigned short)3,1);
                            int sizeOfMsg = sizeof(struct mymsg) - sizeof(long);
                            if(msgsnd(msgQId,&sendByMsgQ,sizeOfMsg,0) == -1)
                                errExit("<Client> Non sono riuscito a inviare un messaggio nella message queue\n");
                            checkinvio[3] = 1;
                            printf("<Client_%d> Ho fatto l'invio in msgQ\n",pid);
                        }
                        else{
                            semOp(mutex, (unsigned short)3, 1);
                        }
                    }

                }while(checkinvio[0] != 1 || checkinvio[1] != 1 || checkinvio[2] != 1 || checkinvio[3] != 1);

                close(fd);
                kill(pid,SIGKILL);
            }
        }
        //Client_0 si mette in attesa di conferma fine lavoro dal server, la conferma sarà un 1 in mtype
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
            errExit("\"<Client_0> Non sono riuscito a riempire il set di segnali\n");
        }
        //aggiunta dei segnali SIGINT e SIGUSR1 alla maschera
        if(sigdelset(&signalSet, SIGINT) == -1){
            errExit("\"<Client_0> Non sono riuscito a togliere SIGINT e SIGUSR1 dal set di segnali\n");
        }
        if(sigdelset(&signalSet, SIGUSR1) == -1){
            errExit("\"<Client_0> Non sono riuscito a togliere SIGINT e SIGUSR1 dal set di segnali\n");
        }
        if(sigprocmask(SIG_SETMASK, &signalSet, NULL) == -1){
            errExit("<Client_0> Non sono riuscito a settare la maschera dei segnali\n");
        }

    }
}
