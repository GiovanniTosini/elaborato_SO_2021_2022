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
    int semIdForFIFO1 = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if(semIdForFIFO1 == -1){
        errExit("failed to create semaphore for FIFO1");
    }
    int semIdForFIFO2 = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if(semIdForFIFO2 == -1){
        errExit("failed to create semaphore for FIFO2");
    }
    int semIdForShMemory = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if(semIdForShMemory == -1){
        errExit("failed to create semaphore for shared memory");
    }
    int semIdForMsgQ = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if(semIdForMsgQ == -1){
        errExit("failed to create semaphore for msg q");
    }

    //apro la FIFO e invio l'ID della shared memory e della msg queue
    int fdDummy = open(fifoDummy, O_RDONLY);
    write(fdDummy,shmid,sizeof(shmid));
    write(fdDummy,msqid,sizeof(msqid));
    write(fdDummy,semIdForFIFO1,sizeof(semIdForFIFO1));
    write(fdDummy,semIdForFIFO2,sizeof(semIdForFIFO2));
    write(fdDummy,semIdForShMemory,sizeof(semIdForShMemory));
    write(fdDummy,semIdForMsgQ,sizeof(semIdForMsgQ));

    close(fdDummy);
    unlink(fdDummy);


    //apro la FIFO1 (si blocca se non l'ha aperta il client)
    int fdfifo1=open(fifo1name,O_RDONLY);

    //leggo il numero di file 
    int nfiles;
    read(fdfifo1,nfiles,sizeof(nfiles));

    //invio conferma al client
    //attach shmemory (il prof lu dichiara void e con dimensione NULL ES.5 N.4)
    //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
    char *shmpiece=(char *)shmat(shmid,NULL,0); //TODO da rifare con funzioni nuove

    //invio conferma su shmemory
    shmpiece[0]="N_FILES RICEVUTO";


    //...


    //detach shmemory
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
