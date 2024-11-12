#include <stdlib.h>
#include "tokenisation.h"
#include <commands.h>
#include <unistd.h>
#include <string.h>


char DELIMITER = ' ';

int is_internal_command(char* command){
    // check if the command is an internal command
    if (strcmp(command, internal_commands[0]) == 0){
        return 1;
    }
    return 0;
}

char* get_external_command_path(char* command){
    char* path = getenv("PATH");
    if (path == NULL){
        perror("error getting the PATH variable");
        return NULL;
    }
    char path_copy = strdup(path);
    if (path_copy == NULL){
        perror("error copying the PATH variable");
        return NULL;
    }
    char* dir = strtok(path_copy, ":");
    while(dir != NULL){
        // check if the command exists in the directory
        dir = strtok(NULL, ":");
        char* fullPath = malloc(strlen(dir) + strlen(command) + 2);
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, command);
        if(access(fullPath, F_OK) == 0){
            free(path_copy);
            return strdup(fullPath);
        }else{
            free(fullPath);
            dir = strtok(NULL, ":");
        }
    }
    free(path_copy);
    return NULL;
}


int run_command(char** command){
    // determine if the command is internal or external
    char* command_name = command[0];
    // search for the command in the list of internal commands
    if (is_internal_command(command_name) == 1){
        // run the internal command
        printf("running internal command %s\n", command_name);
    }else{
        // find the external command
        char* commandPath = get_external_command_path(command_name);
        if (commandPath == NULL){
            return -1; // if the command is not in the path, return -1
        }else{
            pid_t pid = fork();
            if (pid == 0){
                // child process
                execv(commandPath, command); // can be optimiezd by using the execvp function
                perror("error executing the command");
                return -1;
            }else{
                // parent process
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)){
                    return WEXITSTATUS(status);
                }else{
                    return status;
                }
            }
        }
    }
}