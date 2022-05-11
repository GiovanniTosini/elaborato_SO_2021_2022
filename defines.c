/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/msg.h>

//scorre i file, verifica che siano validi e li salva in "*files"
void search(char *files[], char currdir[], int n_files){

	struct dirent *dentry;
    struct stat sb; //struttura di supporto per verificare la dimensione del singolo file
    DIR *dirp = opendir(currdir);

    /* skippa le cartelle '.' e '..'
     * cerca nelle altre cartelle i file
     * che iniziano per 'sendme_'
     * */
    while((dentry = readdir(dirp)) != NULL){

        if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0){
            continue;
        }
        //se è un file ed inizia con 'sendme_' otteniamo il suo path e lo carichiamo in memoria
        if((dentry->d_type == DT_REG) && (strncmp(dentry->d_name, "sendme_", strlen("sendme_")) == 0)){
            char pathname[250]; //pathname attuale
            *pathname = strcat(currdir, dentry->d_name); //FORSE BISOGNA METTERE '/' DOPO IL PATH DELLA CARTELLA
            stat(pathname, &sb); //ottengo info file
            if(sb.st_size <= 4096){ //TODO: oppure 4000
                //il file è minore uguale a 4KB, bisogna SISTEMARE
                files[n_files] = pathname;
                n_files++; 
            }
        }else if(dentry->d_type == DT_DIR){
        	
        	char *dir = strcat(currdir, dentry->d_name);
        	
        	search(files, dir, n_files);
    	}
    }
}

/*funzione che calcola l'incremento necessario per rendere un numero
 * divisibile per 4
 */

int divideBy4(int counter){
    int incremento = 0;
    while(counter % 4 != 0){
        incremento++;
        counter++;
    }
    return incremento;
}
