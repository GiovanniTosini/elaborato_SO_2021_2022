/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include <sys/stat.h>
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

/* salva le parti di messaggio ciclando il buffer fino a n_files, se trova un mtype == 0
 * significa che ha raggiunto l'ultima posizione salvata, quindi in base al valore della
 * variabile ipc, salva nella posizione corretta
 * possibili valori di ipc:
 * - 1 -> FIFO1
 * - 2 -> FIFO2
 * - 3 -> Shared Memory
 * - 4 -> MsgQueue
 */

/* PID del Client_0, fifo1, fifo2, shmId, puntatore alla shm, msgqid
 * dichiarate globalmente per permettere la chiusura
 * in caso di SIGINT
 */
pid_t pidClient_0;
int fdfifo1;
int fdfifo2;
int shmId;
int msQId;
int semIdForIPC;
struct mymsg rcvFromFifo1, rcvFromFifo2, *rcvFromShM, rcvFromMsgQ;
char *fifo1name = "/tmp/myfifo1";
char *fifo2name = "/tmp/myfifo2";

int sizeOfResult;
char pidToStr[8];

void fillTheBuffer(struct mymsg rcvFrom, struct myfile buffer[], int n_files, int ipc);
void serverSigHandler(int sig);

int main() {

    printf("<Server> Avvio...\n");
    //creo la FIFO1
    if(mkfifo(fifo1name,S_IRUSR|S_IWUSR) == -1)
        errExit("<Server> Non sono riuscito a creare la fifo1 a riga 51");
    printf("<Server> Ho creato la FIFO1\n");

    //creo la FIFO2
    if(mkfifo(fifo2name,S_IRUSR|S_IWUSR) == -1)
        errExit("<Server> Non sono riuscito a creare la fifo2 a riga 55");
    printf("<Server> Ho creato la FIFO2\n");

    //creo la shared memory
    shmId = alloc_shared_memory(IPC_PRIVATE, sizeof(struct mymsg) * 50);
    printf("<Server> Ho allocato memoria la shared memory, con id: %d\n", shmId);

    //creo la msg queue TODO cambio da ipc private
    msQId = msgget(IPC_PRIVATE, IPC_CREAT | S_IRUSR | S_IWUSR);
    printf("<Server> Ho creato la coda di messaggi, con id: %d\n", msQId);

    //definizione semafori gestione IPC (max 50 msg per IPC)
    semIdForIPC = semget(IPC_PRIVATE, 4, IPC_CREAT|S_IRUSR | S_IWUSR);
    if(semIdForIPC == -1){
        errExit("<Server> Non sono riuscito a creare il semaforo per le IPC\n");
    }
    printf("<Server> Ho creato il set di semafori per le IPC\n");
    /* FIFO1=1
     * FIFO2=2
     * MSGQueue=3
     * SHdMem=4
     */
    unsigned short values[]={50,50,50,50};
    union semun argIPC;
    argIPC.array = values;


    if(semctl(semIdForIPC,0,SETALL,argIPC) == -1){
        errExit("<Server> Non sono riuscito a settare i semafori delle IPC\n");
    }
    printf("<Server> Ho settato il set dei semafori per le IPC\n");

    //apertura FIFO
    fdfifo1 = open(fifo1name,O_RDONLY);
    fdfifo2 = open(fifo2name, O_RDONLY);

    //ricezione PID di CLient_0
    printf("<Server> Ricezione PID Client_0\n");
    read(fdfifo1, &pidClient_0, sizeof(pidClient_0));
    printf("<Server> Ho ricevuto il PID: %d del Client_0\n", pidClient_0);

    if(signal(SIGINT, serverSigHandler) == SIG_ERR)
        errExit("<Server> Non sono riuscito a settare il signal handler per SIGINT");

    //leggo il numero di file
    int n_files=0;
    //inizializzazione del cursore per shared memory
    //leggi sarà la variabile in cui si salvano i byte letti
    //array di supporto se una IPC viene chiusa il valore sarà modificato a 1 per evitare che il server la usi e si blocchi
    //inoltre serve a far capire se son stati ricomposti tutti i messaggi
    int leggi=0;
    int closedIPC[]={0, 0, 0, 0};
    int cursor = 0;
    int counterForFIFO1,counterForFIFO2,counterShM,counterForMsgQ;
    int sizeOfMessage;


    while(1){

        n_files = 0;
        //printf("\n\nn_files %d pre ricevimento\n\n", n_files);

        read(fdfifo1, &n_files, sizeof(int));


        printf("<Server> Ricevuti %d file con cui lavorare\n", n_files);
        //printf("\n\nn_files %d\n\n", n_files);
        //contatore per FIFO1, FIFO2, shareM, MsgQ
        counterForFIFO1 = n_files;
        counterForFIFO2 = n_files;
        counterShM = n_files;
        counterForMsgQ = n_files;

        //chiusura momentanea per poi riaprirla in scrittura per inviare ID di shm, msgq e semIdForIPC
        close(fdfifo1);
        sleep(5); //dovrebbe evitare che server si blocchi in apertura della fifo
        fdfifo1 = open(fifo1name,O_WRONLY); //TODO metterlo sia in lettura che in scrittura
        if(fdfifo1 == -1)
            errExit("<Server> Errore open Fifo1\n");
        printf("<Server> Ho aperto la fifo1 in scrittura\n");

        printf("<Server> Invio ID della shared memory a Client_0\n");
        write(fdfifo1, &shmId, sizeof(int));
        printf("<Server> Invio ID della message queue a Client_0\n");
        write(fdfifo1, &msQId, sizeof(int));
        printf("<Server> Invio ID del set di semafori a Client_0\n");
        write(fdfifo1,&semIdForIPC,sizeof(int));

        if(close(fdfifo1) == -1)
            errExit("<Server> Non sono riuscito a chiudere la fifo dummy\n");

        fdfifo1 = open(fifo1name, O_RDONLY);
        printf("<Server> Ho riaperto la fifo1 in sola lettura\n");

        //attach shmemory (il prof lo dichiara void e con dimensione NULL ES.5 N.4)
        //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
        rcvFromShM=(struct mymsg*) get_shared_memory(shmId, 0);

        //invio conferma su shmemory
        rcvFromShM[0].mtype = 1;
        printf("<Server> Inviata conferma ricezione numero file a Client_0\n");

        struct myfile buffer[n_files]; //POTREBBE ROMPERE PER QUESTO CHE VIENE DICHIARATO DINAMICAMENTE
        //inizializzo l'array con pid a 0
        printf("<Server> Inizializzo la struttura buffer\n");
        for(int i = 0; i < n_files; i++){
            buffer[i].pid = 0;
        }


        //variabile di aiuto per la dimensione della message queue
        sizeOfMessage = sizeof(struct mymsg) - sizeof(long);

        //INIZIO RICEZIONE
        printf("<Server> Inizio ricezione dei file\n");
        while(closedIPC[0] == 0 || closedIPC[1] == 0 || closedIPC[2] == 0 || closedIPC[3] == 0){
            /* da man 7 pipe, se si prova a leggere da una FIFO che non ha più processi
             * in scrittura la read tornerà 0
             */
            //leggo dalla fifo1 TODO se non trova da leggere si blocca
            if(counterForFIFO1 > 0){
                leggi = read(fdfifo1, &rcvFromFifo1,sizeof(rcvFromFifo1));
                if(leggi == -1) {
                    errExit("<Server> Non sono riuscito a leggere dalla FIFO1\n");
                }
                else {
                    fillTheBuffer(rcvFromFifo1, buffer, n_files, 1);
                    //printf("<Server> Ho salvato il messaggio della FIFO1 del processo %d\n",rcvFromFifo1.pid);
                    semOp(semIdForIPC,0,1);
                    counterForFIFO1--;
                }
            }

            //lettura da FIFO2 TODO se non trova da leggere si blocca
            if(counterForFIFO2 > 0) {
                leggi = read(fdfifo2, &rcvFromFifo2, sizeof(rcvFromFifo2));
                if (leggi == -1) {
                    errExit("<Server> Non sono riuscito a leggere dalla FIFO2\n");
                }
                else {
                    fillTheBuffer(rcvFromFifo2, buffer, n_files, 2);
                    //printf("<Server> Ho salvato il messaggio della FIFO2 del processo %d\n",rcvFromFifo2.pid);
                    semOp(semIdForIPC, 1, 1);
                    counterForFIFO2--;
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
                if(counterShM == 0){
                    closedIPC[2] = 1;
                }
                else if(rcvFromShM[cursor].mtype == 0){
                    //printf("<Server> Verifico la shared memory ricevuta dal PID %d, mtype: %ld\nstringa: %s\npathname: %s\n", rcvFromShM[cursor].pid, rcvFromShM[cursor].mtype, rcvFromShM[cursor].portion, rcvFromShM[cursor].pathname);
                    fillTheBuffer(rcvFromShM[cursor], buffer, n_files, 4);
                    //printf("<Server> Ho salvato il messaggio della shared memory del processo %d\n",rcvFromShM->pid);
                    rcvFromShM[cursor].mtype = 1;
                    //printf("<Server> Ho impostato mtype: %ld cursor ha valore: %d\n", rcvFromShM->mtype, cursor);
                    cursor++;
                    if(cursor == 50){
                        cursor = 0;
                    }
                    //printf("<Server> Cursor: %d dopo l'incremento\n", cursor);
                    counterShM--;
                    semOp(semIdForIPC, 2, 1);
                }
            }

            if(closedIPC[3] == 0){
                //lettura message queue
                if(msgrcv(msQId, &rcvFromMsgQ, sizeOfMessage, 0, IPC_NOWAIT) == -1) {
                    if (errno == ENOMSG)
                        printf("<Server> Message Queue vuota, proseguo oltre\n");
                    else
                        errExit("<Server> Non sono riuscito a ricevere un messaggio dalla message queue\n");
                }
                else {
                    fillTheBuffer(rcvFromMsgQ, buffer, n_files, 3);
                    //printf("<Server> Ho salvato il messaggio della msgQ del processo %d\n",rcvFromMsgQ.pid);
                    counterForMsgQ--;
                    if(counterForMsgQ == 0)
                        closedIPC[3] = 1;
                    semOp(semIdForIPC, 3, 1);
                }
            }
            if(counterForFIFO1 == 0)
                closedIPC[0] = 1;
            if(counterForFIFO2 == 0)
                closedIPC[1] = 1;
        }

        printf("<Server> Sto per ricostruire e scrivere su file i messaggi\n");
        //Ricostruzione messaggi
        for(int i = 0; i < n_files; i++){
            //preparo le lunghezze delle 4 parti per le future write e il cast a str del pid
            /*int lenOfFIFO1 = strlen(buffer[i].fifo1);
            int lenOfFIFO2 = strlen(buffer[i].fifo2);
            int lenOfShM = strlen(buffer[i].shMem);
            int lenOfMsgQ = strlen(buffer[i].msgQ);*/

            sprintf(pidToStr, "%d", buffer[i].pid);
            //ottengo il pathname completo del file di out (quello con _out) alla fine
            char *newName = strcat(buffer[i].pathname, "_out"); //TODO da convertire in array
            int fdNewFile = open(newName, O_WRONLY | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
            if(fdNewFile == -1)
                errExit("<Server> Non sono riuscito a creare un nuovo file\n");
            //con O_APPEND ad ogni write verrà tutto messo in fondo, nessuna sovrascrittura
            //ssize_t resultWrite; //lo userò per ogni write e per verificare che sia andata a buon fine
            //TODO valutare se mettere i controlli dell'avvenuta write o meno
            char *newNewName = strcat(newName, ":\n");
            write(fdNewFile, newNewName, strlen(newNewName));
            //PARTE 1 TODO sistemare le stampe: riga 269,...
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
            close(fdNewFile);
        }

        //TODO la chiusura delle IPC avviene solo alla ricezione di un SIG_INT

        //invio conferma fine lavoro
        rcvFromMsgQ.mtype = 1;
        sizeOfResult = sizeof(struct mymsg) - sizeof(long);
        if(msgsnd(msQId, &rcvFromMsgQ, sizeOfResult, 0) == -1)
            errExit("<Server> Non sono riuscito a inviare la conferma di lavoro concluso\n");
        printf("<Server> Torno in attesa...\n");

        leggi=0;
        closedIPC[0]=0;
        closedIPC[1]=0;
        closedIPC[2]=0;
        closedIPC[3]=0;
        cursor=0;
        n_files = 0;


    }
}

void fillTheBuffer(struct mymsg rcvFrom, struct myfile buffer[], int n_files, int ipc){

    int i = 0;
    while(i < n_files){
        if(buffer[i].pid == 0){
            buffer[i].pid = rcvFrom.pid;
            strcpy(buffer[i].pathname, rcvFrom.pathname);
            switch (ipc) {
                case 1:
                    strcpy(buffer[i].fifo1, rcvFrom.portion);
                    break;
                case 2:
                    strcpy(buffer[i].fifo2, rcvFrom.portion);
                    break;
                case 3:
                    strcpy(buffer[i].msgQ, rcvFrom.portion);
                    break;
                case 4:
                    strcpy(buffer[i].shMem, rcvFrom.portion);
                    break;
                default:
                    break;
            }
            break;
        }else if(buffer[i].pid == rcvFrom.pid){
            switch (ipc) {
                case 1:
                    strcpy(buffer[i].fifo1, rcvFrom.portion);
                    break;
                case 2:
                    strcpy(buffer[i].fifo2, rcvFrom.portion);
                    break;
                case 3:
                    strcpy(buffer[i].msgQ, rcvFrom.portion);
                    break;
                case 4:
                    strcpy(buffer[i].shMem, rcvFrom.portion);
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
        if(kill(pidClient_0, SIGKILL) == -1){
            errExit("<Server> Ops, il Client_0 è sopravvissuto\n");
        }
        close(fdfifo1);
        unlink(fifo1name);
        close(fdfifo2);
        unlink(fifo2name);
        free_shared_memory(rcvFromShM);
        remove_shared_memory(shmId);
        msgctl(msQId, IPC_RMID, 0);
        semctl(semIdForIPC, 0, IPC_RMID, 0);
        if(kill(getpid(), SIGTERM) == -1)
            errExit("<Server> Non sono riuscito a fare seppuku\n");
    }
}
