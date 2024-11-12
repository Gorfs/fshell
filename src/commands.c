#include <stdlib.h>
#include <tokenisation.h>
#include <commands.h>
#include <unistd.h>
#include <string.h>
// temporary to use printf for debugging
#include <stdio.h>

// list of internal commands
char* internal_commands[] = { // Array of string pointers
        "cd",
        "exit",
        "pwd",
        NULL // Important: NULL terminator if you want to iterate easily
    };


int is_internal_command(char* command_name){
    // check if the command is an internal command (just checking one for now)
    if (strcmp(command_name, internal_commands[0]) == 0){
        return 1;
    }
    return 0;
}

void run_command(char** command){
    char* command_name = command[0];
    // determine if the command is internal or external
    if (is_internal_command(command_name) == 1){
        // run the internal command
        printf("running internal command %s\n", command_name);
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

