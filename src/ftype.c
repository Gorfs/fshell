#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

int command_ftype(char** command){
    struct stat st;
    if(lstat(command[1], &st) == 0){
        if(S_ISLNK(st.st_mode)){
            write(1, "symbolic link\n", 14);
        }
        else if(S_ISDIR(st.st_mode)){
            write(1, "directory\n", 10);
        }
        else if(S_ISREG(st.st_mode)){
            write(1, "regular file\n", 13);
        }
        else if(S_ISFIFO(st.st_mode)){
            write(1, "named pipe\n", 11);
        }
        else{
            write(1, "other\n", 6);
        }
    }
    else{
        write(STDERR_FILENO, "ftype: No such file or directory\n", 33);
        return 1;
    }
    return 0;
}
