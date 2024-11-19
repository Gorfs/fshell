#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int command_cd(char** command){

    if (command[1] == NULL || strcmp(command[1], "~") == 0 || command[1][0] == '\0'){
        const char *home = getenv("HOME");
        if (chdir(home) == 0){
            return 0;
        }else{
            return 1;
        }
    }
    if (chdir(command[1]) == 0){
        return 0;
    }else{
        return 1;
    }
}
