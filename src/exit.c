#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <exit.h>

int command_exit(char** command, int last_val){
    if(command == NULL || command[1] == NULL){
        if (command != NULL){
            free(command);
        }
        exit(last_val);
    }
    else{
        int exit_code = atoi(command[1]);
        free(command);
        exit(exit_code);
    }
}