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
#include <signal.h>

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

/* PID del Client_0, fifo1, fifo2, shmid, puntatore alla shm, msgqid
 * dichiarate globalmente per permettere la chiusura
 * in caso di SIGINT
 */
pid_t pidClient_0;
int fdfifo1;
int fdfifo2;
int shmid;
int msqid;
struct mymsg *rcvFromFifo1, *rcvFromFifo2, *rcvFromShM, *rcvFromMsgQ;

void fillTheBuffer(struct mymsg rcvFrom, struct myfile buffer[], int n_files, int ipc);
void serverSigHandler(int sig);

int main() {
    char *fifo1name = "/tmp/myfifo1";
    char *fifo2name = "/tmp/myfifo2";
    char *fifoDummy = "/tmp/myFifoDummy";

    //creo la FIFO1
    int fifo1 = mkfifo(fifo1name,S_IRUSR|S_IWUSR);
    
    //creo la FIFO2
    int fifo2 = mkfifo(fifo2name,S_IRUSR|S_IWUSR);
    
    //creo la shared memory
    shmid = alloc_shared_memory(IPC_PRIVATE, sizeof(struct mymsg) * 50);
    
    //creo la msg queue
    msqid = msgget(IPC_PRIVATE,IPC_CREAT|S_IRUSR|S_IWUSR);

    //definizione semafori gestione IPC (max 50 msg per IPC)
    int semIdForIPC = semget(IPC_PRIVATE, 4, IPC_CREAT|S_IRUSR | S_IWUSR);
    if(semIdForIPC == -1){
        errExit("<Server> Non sono riuscito a creare il semaforo per le IPC\n");
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
        errExit("<Server> Non sono riuscito a settare i semafori delle IPC\n");
    }

    //apertura FIFO
    fdfifo1 = open(fifo1name,O_RDONLY);
    fdfifo2 = open(fifo2name, O_RDONLY);

    //attach shmemory (il prof lo dichiara void e con dimensione NULL ES.5 N.4)
    //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
    rcvFromShM = (struct mymsg*) get_shared_memory(shmid, 0);

    //ricezione PID di CLient_0
    read(fdfifo1, pidClient_0, sizeof(pidClient_0));

    if(signal(SIGINT, serverSigHandler) == SIG_ERR)
        errExit("<Server> Non sono riuscito a settare il signal handler per SIGINT");

    while(1){

        //leggo il numero di file
        int n_files;
        read(fdfifo1, n_files, sizeof(n_files));
        printf("<Server> ricevuti %d file con cui lavorare", n_files);
        int counterForMsgQ = n_files;

        //apro la FIFO e invio l'ID della shared memory e della msg queue
        int fifoD = mkfifo(fifoDummy, S_IRUSR|S_IWUSR);
        int fdDummy = open(fifoDummy, O_WRONLY);
        write(fdDummy,shmid,sizeof(shmid));
        write(fdDummy,msqid,sizeof(msqid));
        write(fdDummy,semIdForIPC,sizeof(semIdForIPC));

        close(fdDummy);
        unlink(fdDummy);

        //invio conferma su shmemory
        rcvFromShM[0].mtype = 1;
        printf("<Server> invio conferma ricezione numero file a Client_0");

        struct myfile buffer[n_files]; //POTREBBE ROMPERE LE PALLE PER QUESTO CHE VIENE DICHIARATO DINAMICAMENTE
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
        //inoltre serve a far capire se son stati ricomposti tutti i messaggi
        int cursor = 0, leggi, closedIPC[] = {0, 0, 0, 0};
        //variabile di aiuto per la dimensione della message queue
        int sizeOfMessage = sizeof(struct mymsg) - sizeof(long);

        //INIZIO RICEZIONE
        while(closedIPC[0] == 0 || closedIPC[1] == 0 || closedIPC[2] == 0 || closedIPC[3] == 0){
            /* da man 7 pipe, se si prova a leggere da una FIFO che non ha più processi
             * in scrittura la read tornerà 0
             */
            //leggo dalla fifo1 TODO se non trova da leggere si blocca
            if(closedIPC[0] == 0){
                leggi = read(fdfifo1,rcvFromFifo1,sizeof(rcvFromFifo1));
                if(leggi == -1) {
                    errExit("<Server> Non sono riuscito a leggere dalla FIFO1\n");
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
                    errExit("<Server> Non sono riuscito a leggere dalla FIFO2\n");
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
                if(rcvFromShM[cursor].mtype == 1){
                    closedIPC[2] = 1;
                }
                else if(rcvFromShM[cursor].mtype != 0){
                    fillTheBuffer(rcvFromShM[cursor], buffer, n_files, 3);
                    rcvFromMsgQ[cursor].mtype = 1;
                    cursor++;
                    if(cursor == 50){
                        cursor = 0;
                    }
                    semOp(semIdForIPC, 3, 1);
                }
            }

            if(closedIPC[3] == 0){
                //lettura message queue TODO mettere un contatore di volte in cui si è trovata la msgQ vuota, arrivato a quello si esce dal ciclo delle letture?
                if(msgrcv(msqid, rcvFromMsgQ, sizeOfMessage, 0, IPC_NOWAIT) == -1) {
                    if (errno == ENOMSG)
                        printf("<Server> Message Queue vuota, proseguo oltre");
                    else
                        errExit("<Server> Non sono riuscito a ricevere un messaggio dalla message queue\n");
                }
                else {
                    fillTheBuffer(*rcvFromMsgQ, buffer, n_files, 4);
                    semOp(msqid, 4, 1);
                    counterForMsgQ--;
                    if(counterForMsgQ == 0)
                        closedIPC[3] = 1;
                }
            }
        }

        //Ricostruzione messaggi
        for(int i = 0; i < n_files; i++){
            //preparo le lunghezze delle 4 parti per le future write e il cast a str del pid
            /*int lenOfFIFO1 = strlen(buffer[i].fifo1);
            int lenOfFIFO2 = strlen(buffer[i].fifo2);
            int lenOfShM = strlen(buffer[i].shMem);
            int lenOfMsgQ = strlen(buffer[i].msgQ);*/
            char pidToStr[8];
            sprintf(pidToStr, "%ld", buffer[i].pid);
            //ottengo il pathname completo del file di out (quello con _out) alla fine
            char *newName = strcat(buffer[i].pathname, "_out");
            int fdNewFile = open(newName, O_WRONLY | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
            if(fdNewFile == -1)
                errExit("<Server> Non sono riuscito a creare un nuovo file\n");
            //con O_APPEND ad ogni write verrà tutto messo in fondo, nessuna sovrascrittura
            ssize_t resultWrite; //lo userò per ogni write e per verificare che sia andata a buon fine
            //TODO valutare se mettere i controlli dell'avvenuta write o meno
            char *newNewName = strcat(newName, ":\n");
            write(fdNewFile, newNewName, strlen(newNewName));
            //PARTE 1
            write(fdNewFile, "[Parte 1, del file ", strlen("[Parte 1, del file "));
            write(fdNewFile, buffer[i].pathname, strlen(buffer[i].pathname));
            write(fdNewFile, ", spedita dal processo ", strlen(", spedita dal processo "));
            write(fdNewFile, pidToStr, sizeof(pidToStr));
            write(fdNewFile, " tramite FIFO1]\n", strlen(" tramite FIFO1]\n"));
            write(fdNewFile, buffer[i].fifo1, strlen(buffer[i].fifo1));
            //PARTE 2
            write(fdNewFile, "\n\n[Parte 2, del file ", strlen("\n\n[Parte 2, del file "));
            write(fdNewFile, buffer[i].pathname, strlen(buffer[i].pathname));
            write(fdNewFile, ", spedita dal processo ", strlen(", spedita dal processo "));
            write(fdNewFile, pidToStr, sizeof(pidToStr));
            write(fdNewFile, " tramite FIFO2]\n", strlen(" tramite FIFO2]\n"));
            write(fdNewFile, buffer[i].fifo2, strlen(buffer[i].fifo2));
            //PARTE 3 //TODO msgQ va per terza magari meglio ordinare visivamente l'invio e la ricezione
            write(fdNewFile, "\n\n[Parte 3, del file ", strlen("\n\n[Parte 3, del file "));
            write(fdNewFile, buffer[i].pathname, strlen(buffer[i].pathname));
            write(fdNewFile, ", spedita dal processo ", strlen(", spedita dal processo "));
            write(fdNewFile, pidToStr, sizeof(pidToStr));
            write(fdNewFile, " tramite MsgQueue]\n", strlen(" tramite MsgQueue]\n"));
            write(fdNewFile, buffer[i].msgQ, strlen(buffer[i].msgQ));
            //PARTE 4
            write(fdNewFile, "\n\n[Parte 4, del file ", strlen("\n\n[Parte 4, del file "));
            write(fdNewFile, buffer[i].pathname, strlen(buffer[i].pathname));
            write(fdNewFile, ", spedita dal processo ", strlen(", spedita dal processo "));
            write(fdNewFile, pidToStr, sizeof(pidToStr));
            write(fdNewFile, " tramite ShdMem]\n", strlen(" tramite ShdMem]\n"));
            write(fdNewFile, buffer[i].shMem, strlen(buffer[i].shMem));
        }

        //TODO la chiusura delle IPC avviene solo alla ricezione di un SIG_INT

        //invio conferma fine lavoro
        rcvFromMsgQ->mtype = 1;
        int sizeOfResult = sizeof(struct mymsg) - sizeof(long);
        if(msgsnd(msqid, &rcvFromMsgQ, sizeOfResult, 0) == -1)
            errExit("<Server> Non sono riuscito a inviare la conferma di lavoro concluso\n");
    }
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

void serverSigHandler(int sig) {
    if(sig == SIGINT){
        printf("<Server> Terminazione del processo Client_0.\n");
        if(kill(pidClient_0, SIGTERM) == -1){
            errExit("<Server> Ops, il Client_0 è sopravvissuto");
        }
        close(fdfifo1);
        unlink(fdfifo1);
        close(fdfifo2);
        unlink(fdfifo2);
        free_shared_memory(rcvFromShM);
        remove_shared_memory(shmid);
        msgctl(msqid, IPC_RMID, 0);
        if(kill(getpid(), SIGTERM) == -1)
            errExit("<Server> Non sono riuscito a fare seppuku");
    }
}
