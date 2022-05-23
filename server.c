/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sem.h>
#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

int main(int argc, char * argv[]) {
    char *fifo1name = "/tmp/myfifo1";
    char *fifo2name = "/tmp/myfifo2";
    char *fifoDummy = "/tmp/myFifoDummy";

    //creo la FIFO1
    //creata,aperta, chiusa ed eliminata: ok
    int fifo1=mkfifo(fifo1name,S_IRUSR|S_IWUSR);
    
    //creo la FIFO2
    //creata,chiusa ed eliminata: manca apertura
    int fifo2=mkfifo(fifo2name,S_IRUSR|S_IWUSR);
    
    //creo la shared memory
    //creata,attach, detach, eliminata: ok
    int shmid = alloc_shared_memory(IPC_PRIVATE, sizeof(struct mymsg) * 50);
    
    //creo la msg queue
    int msqid=msgget(IPC_PRIVATE,IPC_CREAT|S_IRUSR|S_IWUSR);

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


    //apro la FIFO1 (si blocca se non l'ha aperta il client)
    int fdfifo1=open(fifo1name,O_RDONLY);

    //leggo il numero di file 
    int nfiles;
    read(fdfifo1,nfiles,sizeof(nfiles));

    //invio conferma al client
    //attach shmemory (il prof lo dichiara void e con dimensione NULL ES.5 N.4)
    //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
    struct mymsg *shmpiece=(struct mymsg*) get_shared_memory(shmid, 0);

    //invio conferma su shmemory
    shmpiece->mtype=1;

    struct mymsg *rcvFromFifo1,*rcvFromFifo2,*rcvFromShM,*rcvFromMsgQ;
    struct myfile buffer[nfiles];
    while(1){
        //leggo dalla fifo1
        int leggi=read(fifo1,rcvFromFifo1,sizeof(rcvFromFifo1));
        if(leggi==-1)
            errExit("read error!");
        else if(leggi>0)

    }

    //detach shmemory TODO da sistemare con metodi specifici
    if(shmdt(message)==-1)
        errExit("shmdt failed");

    //elimino la shared memory
    if(shmctl(shmid,IPC_RMID,NULL)==-1)
        errExit("errore rimozione shared memory");

    //elimino FIFO1
    close(fdfifo1);
    unlink(fifo1);

    //elimino FIFO2
    close(fdfifo2);
    unlink(fifo2);


    //elimino msgqueue
    if(msgctl(msqid,IPC_RMID,NULL)==-1)
        errExit("errore rimozione message queue");


}
