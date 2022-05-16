/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

int main(int argc, char * argv[]) {
    char *fname1="/tmp/myfifo1";
    char *fname2="/tmp/myfifo2";
    char *fnameSh="/tmp/myFifoForId"

    //creo la FIFO1
    //creata,aperta, chiusa ed eliminata: ok
    int fifo1=mkfifo(fname1,S_IRUSR|S_IWUSR);
    //creo la FIFO2
    //creata,chiusa ed eliminata: manca apertura
    int fifo2=mkfifo(fname2,S_IRUSR|S_IWUSR);
    //creo la shared memory (DIMENSIONE DA DEFINIRE???)
    //creata,attach, detach, eliminata: ok
    int shmid=shmget(IPC_PRIVATE,1050*50,S_IRUSR | S_IWUSR);
    //creo la msg queue
    int msqid=msgget(IPC_PRIVATE,IPC_CREAT|S_IRUSR|S_IWUSR);

    //apro la FIFO e invio l'ID della shared memory e della msg queue
    int fdsh=open(fnameSh,O_RDONLY);
    write(fdsh,shmid,sizeof(shmid));
    write(fdsh,msqid,sizeof(msqid));


    //apro la FIFO1 (si blocca se non l'ha aperta il client)
    int fdfifo1=open(fname1,O_RDONLY);

    //leggo il numero di file 
    int nfiles;
    read(fdfifo1,nfiles,sizeof(nfiles));

    //invio conferma al client
    //attach shmemory (il prof lu dichiara void e con dimensione NULL ES.5 N.4)
    //PROBLEMA: è possibile tenere le flag per read/write, solo read ma non solo write...
    char *shmpiece=(char *)shmat(shmid,NULL,0);

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
    
    //elimino FIFOFORID?


}
