/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

/* salva le parti di messaggio ciclando il buffer fino a n_files, se trova un mtype == 0
 * significa che ha raggiunto l'ultima posizione salvata, quindi in base al valore della
 * variabile ipc, salva nella posizione corretta
 * possibili valori di ipc:
 * - 1 -> FIFO1
 * - 2 -> FIFO2
 * - 3 -> Shared Memory
 * - 4 -> MsgQueue
 */
void fillTheBuffer(struct mymsg rcvFrom, struct myfile buffer[], int n_files, int ipc);

int main(int argc, char * argv[]) {
    char *fifo1name = "/tmp/myfifo1";
    char *fifo2name = "/tmp/myfifo2";
    char *fifoDummy = "/tmp/myFifoDummy";

    //creo la FIFO1
    //creata,aperta, chiusa ed eliminata: ok
    int fifo1 = mkfifo(fifo1name,S_IRUSR|S_IWUSR);
    
    //creo la FIFO2
    //creata,chiusa ed eliminata: manca apertura
    int fifo2 = mkfifo(fifo2name,S_IRUSR|S_IWUSR);
    
    //creo la shared memory
    //creata,attach, detach, eliminata: ok
    int shmid = alloc_shared_memory(IPC_PRIVATE, sizeof(struct mymsg) * 50);
    
    //creo la msg queue
    int msqid = msgget(IPC_PRIVATE,IPC_CREAT|S_IRUSR|S_IWUSR);

    //definizione semafori gestione IPC (max 50 msg per IPC)
    int semIdForIPC = semget(IPC_PRIVATE, 4, IPC_CREAT|S_IRUSR | S_IWUSR);

    if(semIdForIPC == -1){
        errExit("failed to create semaphore for FIFO1");
    }
    /* FIFO1=1
     * FIFO2=2
     * MSGQueue=3
     * SHdMem=
     */
    int values[]={50,50,50,50};
    union semun argIPC;
    argIPC.array=values;

    if(semctl(semIdForIPC,0,SETALL,argIPC)==-1){
        errExit("semctl SETALL");
    }


    //apro la FIFO e invio l'ID della shared memory e della msg queue
    int fdDummy = open(fifoDummy, O_RDONLY);
    write(fdDummy,shmid,sizeof(shmid));
    write(fdDummy,msqid,sizeof(msqid));
    write(fdDummy,semIdForIPC,sizeof(semIdForIPC));

    close(fdDummy);
    unlink(fdDummy);


    //apertura FIFO
    int fdfifo1 = open(fifo1name,O_RDONLY);
    int fdfifo2 = open(fifo2name, O_RDONLY);

    //leggo il numero di file 
    int n_files;
    read(fdfifo1, n_files, sizeof(n_files));
    printf("<Server> ricevuti %d file con cui lavorare", n_files);

    //invio conferma al client
    //attach shmemory (il prof lo dichiara void e con dimensione NULL ES.5 N.4)
    //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
    struct mymsg *shmpiece = (struct mymsg*) get_shared_memory(shmid, 0);

    //invio conferma su shmemory
    shmpiece->mtype=1;
    printf("<Server> invio conferma ricezione numero file a Client_0");
    free_shared_memory(shmpiece);

    //inizializzazione strutture per ricezione parti del messaggio e struttura finale per ricostruzione messaggio
    struct mymsg *rcvFromFifo1,*rcvFromFifo2,*rcvFromShM,*rcvFromMsgQ;
    struct myfile buffer[n_files];
    rcvFromShM = (struct mymsg*) get_shared_memory(shmid, 0);
    int index = 0; //indice array buffer
    //inizializzo l'array con pid a 0
    for(int i = 0; i < n_files; i++){
        buffer[i].pid = 0;
        buffer[i].fifo1 = NULL;
        buffer[i].fifo2 = NULL;
        buffer[i].shMem = NULL;
        buffer[i].msgQ = NULL;
    }
    //inizializzazione del cursore per shared memory
    //leggi sarà la variabile in cui si salvano i byte letti
    //array di supporto se una IPC viene chiusa il valore sarà modificato a 1 per evitare che il server la usi e si blocchi
    int cursor = 0, leggi, closedIPC[] = {0, 0, 0, 0};
    //variabile di aiuto per la dimensione della message queue
    int sizeOfMessage = sizeof(struct mymsg) - sizeof(long);


    //INIZIO RICEZIONE
    while(1){
        /* da man 7 pipe, se si prova a leggere da una FIFO che non ha più processi
         * in scrittura la read tornerà 0
         */
        //leggo dalla fifo1 TODO se non trova da leggere si blocca
        if(closedIPC[0] == 0){
            leggi = read(fdfifo1,rcvFromFifo1,sizeof(rcvFromFifo1));
            if(leggi == -1) {
                errExit("read error!");
            }
            else if(leggi == 0){ //tutti i client hanno scritto ed è stato letto tutto chiusura FIFO
                closedIPC[0] = 1;
            }
            else {
                fillTheBuffer(*rcvFromFifo1, buffer, n_files, 1);
                semOp(semIdForIPC,1,1);
            }
        }

        //lettura da FIFO2 TODO se non trova da leggere si blocca
        if(closedIPC[1] == 0) {
            leggi = read(fdfifo2, rcvFromFifo2, sizeof(rcvFromFifo2));
            if (leggi == -1) {
                errExit("read error!");
            }
            else if (leggi == 0) {
                closedIPC[1] = 1;
            }
            else {
                fillTheBuffer(*rcvFromFifo2, buffer, n_files, 2);
                semOp(semIdForIPC, 2, 1);
            }
        }
        //lettura shared Memory
        /* ogni volta che server legge, scrive un 1 su mtype
         * se andando a guardare il valore di mtype trova 1
         * significa che nessun altro client ha scritto
         * e che quindi hanno tutti finito
         * allora server farà detach e remove della shared memory
         */
        if(closedIPC[2] == 0){
            if(rcvFromMsgQ[cursor].mtype == 1){
                closedIPC[2] = 1;
            }
            else if(rcvFromShM[cursor].mtype != 0){
                fillTheBuffer(rcvFromShM[cursor], buffer, n_files, 3);
                rcvFromMsgQ[cursor].mtype = 1;
                cursor += sizeof(struct mymsg);
                if(cursor >= sizeof(struct mymsg) * 50){
                    cursor = 0;
                }
                semOp(semIdForIPC, 3, 1);
            }
        }


        //lettura message queue TODO mettere un contatore di volte in cui si è trovata la msgQ vuota, arrivato a quello si esce dal ciclo delle letture?
        if(msgrcv(msqid, rcvFromMsgQ, sizeOfMessage, 0, IPC_NOWAIT) == -1) {
            if (errno = ENOMSG)
                printf("Message Queue vuota, proseguo oltre");
            else
                errExit("failed to recieve message from message queue");
        }
        else {
            fillTheBuffer(*rcvFromMsgQ, buffer, n_files, 4);
            semOp(msqid, 4, 1);
        }
        //creazione file di output dopo ogni lettura si controlla se si son ricevute le 4 parti di un file TODO
        for(int i = 0; i < n_files; i++){
            if(buffer[i].fifo1 != NULL && buffer[i].fifo2 != NULL && buffer[i].shMem != NULL && buffer[i].msgQ != NULL){
                char *newFile = strcat(buffer[i].pathname, "_out");
                int fdNewFile = open(newFile, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
                if(fdNewFile == -1)
                    errExit("<Server> error while making of new file");


            }
        }

    }

    //TODO la creazione del file di output deve avvenire come vengono ricevute 4 parti di un file.
    //TODO la chiusura delle IPC avviene solo alla ricezione di un SIG_INT

    //invio conferma fine lavoro
    rcvFromMsgQ->mtype = 1;
    int sizeOfResult = sizeof(struct mymsg) - sizeof(long);
    if(msgsnd(msqid, &rcvFromMsgQ, sizeOfResult, 0) == -1)
        errExit("failed to send confirmation end of work");

    //TODO valutare come evitare che server elimini la msgQ prima client legga la conclusione lavori avvenuta

    //elimino msgqueue
    if(msgctl(msqid,IPC_RMID,NULL)==-1)
        errExit("errore rimozione message queue");


}

void fillTheBuffer(struct mymsg rcvFrom, struct myfile buffer[], int n_files, int ipc){

    int i = 0;
    while(i < n_files){
        if(buffer[i].pid == 0){
            buffer[i].pid = rcvFrom.mtype;
            buffer[i].pathname = rcvFrom.pathname;
            switch (ipc) {
                case 1:
                    buffer[i].fifo1 = rcvFrom.portion;
                    break;
                case 2:
                    buffer[i].fifo2 = rcvFrom.portion;
                    break;
                case 3:
                    buffer[i].shMem = rcvFrom.portion;
                    break;
                case 4:
                    buffer[i].msgQ = rcvFrom.portion;
                    break;
                default:
                    break;
            }
            break;
        }else if(buffer[i].pid == rcvFrom.mtype){
            switch (ipc) {
                case 1:
                    buffer[i].fifo1 = rcvFrom.portion;
                    break;
                case 2:
                    buffer[i].fifo2 = rcvFrom.portion;
                    break;
                case 3:
                    buffer[i].shMem = rcvFrom.portion;
                    break;
                case 4:
                    buffer[i].msgQ = rcvFrom.portion;
                    break;
                default:
                    break;
            }
            break;
        }
        i++;
    }
}
