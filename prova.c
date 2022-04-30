#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include "defines.h"
#include "err_exit.h"
int main(){
    DIR *dp = opendir("/home/alessandro/Scrivania");
if (dp == NULL) return -1;
struct dirent *dentry;
// Iterate until NULL is returned as a result.
while ( (dentry = readdir(dp)) != NULL ) {
if (dentry->d_type == DT_REG)
printf("Regular file: %s\n", dentry->d_name);

}
// NULL is returned on error, and when the end-of-directory is reached!

closedir(dp);
}
