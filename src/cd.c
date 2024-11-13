#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>


int command_cd(char** command){
    if (chdir(command[1]) == 0){
        return -1;
    }else{
        return 0;
    }
}