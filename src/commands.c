#include <stdlib.h>
#include <tokenisation.h>
#include <commands.h>
#include <unistd.h>
#include <string.h>
#include "exit.h"
#include "pwd.h"
// temporary to use printf for debugging
#include <stdio.h>

// list of internal commands
char* internal_commands[] = {"exit", "pwd", NULL};

int is_internal_command(char* command_name){
    // check if the command is an internal command (just checking one for now)

    char* command_to_check = internal_commands[0];
    int i = 0;
    while(command_to_check[i]){
        if (strcmp(command_name, command_to_check) == 0){
            return 1;
        };
        i++;
    }
    return 0;
}

void run_command(char** command){
    char* command_name = command[0];
    // determine if the command is internal or external
    if (is_internal_command(command_name) == 1){
        // run the internal command
        // there must be a cleaner way to do this, this is just sad.
        if(strcmp(command_name, "exit") == 0){
            command_exit(0);
        }else if (strcmp(command_name, "pwd") == 0){
            command_pwd(1);
        }else{
            // error handling
            perror("error running internal command");
        }
    }else{
        // make a new process
        pid_t pid = fork();
        if (pid == 0){
            // child process
            execvp(command_name, command);

            // if the execvp function returns, there was an error:
            perror("error executing the command");
        }else{
            // parent process
            int status;
            waitpid(pid, &status, 0);
            // setting env status
            char* status_str = malloc(4);
            sprintf(status_str, "%d", status);
            setenv("?", status_str, 1);
            free(status_str);
       }
    }
}
// not requried but could be useful later down the line
// char* get_external_command_path(char* command){
//     char* path = getenv("PATH");
//     if (path == NULL){
//         perror("error getting the PATH variable");
//         return NULL;
//     }
//     char path_copy = strdup(path);
//     if (path_copy == NULL){
//         perror("error copying the PATH variable");
//         return NULL;
//     }
//     char* dir = strtok(path_copy, ":");
//     while(dir != NULL){
//         // check if the command exists in the directory
//         dir = strtok(NULL, ":");
//         char* fullPath = malloc(strlen(dir) + strlen(command) + 2);
//         snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, command);
//         if(access(fullPath, F_OK) == 0){
//             free(path_copy);
//             return strdup(fullPath);
//         }else{
//             free(fullPath);
//             dir = strtok(NULL, ":");
//         }
//     }
//     free(path_copy);
//     return NULL;
// }

