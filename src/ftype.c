#include <stdlib.h> 
#include <unistd.h>
#include <sys/stat.h>
// include our own files
#include <ftype.h>


/**
 * @brief Function that checks the type of a file.
 * @param command the command to run
 * @return 0 if successful, 1 otherwise
 */
int command_ftype(char** command) {
    struct stat st;
    if(lstat(command[1], &st) == 0){
        if(S_ISLNK(st.st_mode)){
            write(STDOUT_FILENO, "symbolic link\n", 14);
        }
        else if(S_ISDIR(st.st_mode)){
            write(STDOUT_FILENO, "directory\n", 10);
        }
        else if(S_ISREG(st.st_mode)){
            write(STDOUT_FILENO, "regular file\n", 13);
        }
        else if(S_ISFIFO(st.st_mode)){
            write(STDOUT_FILENO, "named pipe\n", 11);
        }
        else{
            write(STDOUT_FILENO, "other\n", 6);
        }
    }
    else{
        write(STDERR_FILENO, "ftype: No such file or directory\n", 33);
        return 1;
    }
    return 0;
}
