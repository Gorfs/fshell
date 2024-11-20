#include <stdlib.h>
#include <tokenisation.h>
#include <sys/wait.h>
#include <commands.h>
#include <unistd.h>
#include <string.h>
#include <exit.h>
#include <pwd.h>
#include <ftype.h>
#include <cd.h>
// temporary to use printf for debugging
#include <stdio.h>

// list of internal commands
char* internal_commands[] = {"exit", "pwd", "cd", "ftype", NULL};

int is_internal_command(char* command_name){
    // check if the command is an internal command (just checking one for now)
    char* command_to_check = internal_commands[0];
    int i = 0;
    while(command_to_check){
        if (strcmp(command_name, command_to_check) == 0){
            return 1;
        };
        command_to_check = internal_commands[i++];
    }
    return 0;
}

int run_command(char** command, int last_val){
    int status = last_val;
    char* command_name = command[0];
    // determine if the command is internal or external
    if (is_internal_command(command_name) == 1){
        // run the internal command
        // there must be a cleaner way to do this, this is just sad.
        if(strcmp(command_name, "exit") == 0){
            command_exit(command, status);
        }else if (strcmp(command_name, "pwd") == 0){
            status = command_pwd(1);
        }else if (strcmp(command_name, "cd") == 0){
            status = command_cd(command);
            if (status == 1){
                write(STDERR_FILENO, "cd: No such file or directory\n", 31);
            }
        }else if (strcmp(command_name, "ftype") == 0){
            status = command_ftype(command);
            if (status == 1){
                write(STDERR_FILENO, "ftype: invalid arguments\n", 26);
            }
        }else{
            // error handling
            perror("error running internal command");
            status = 1;
        }
    }else{
        // make a new process
        pid_t pid = fork();
        if (pid == 0){
            // child process
            execvp(command_name, command);
            exit(1);
            // if the execvp function returns, there was an error:
            perror("error executing the command");
        }else{
            // parent process
            int status;
            waitpid(pid, &status, 0);
            status = WEXITSTATUS(status);
       }
    }

    return status;
}

int run_commands(char*** commands, int last_val, char* input){
    int stdin_copy = dup(STDIN_FILENO);
    int stderr_cpy = dup(STDERR_FILENO);
    int stdout_cpy = dup(STDOUT_FILENO);
    for (int i = 0; commands[i] != NULL; i++){
        last_val = run_command(commands[i], last_val);
    }
}