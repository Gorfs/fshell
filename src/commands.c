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
#include <dirent.h>

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

int len_command(char** tokens){
    int i = 0;
    while(tokens[i]){
        i++;
    }
    return i;
}

char** list_path_files(char* path){
    DIR *dir;
    struct dirent *entry;
    char** files = NULL;
    int size = 0;
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return NULL;
    }

    while((entry = readdir(dir)) != NULL){
        if (entry->d_name[0] != '.' && entry->d_type == DT_REG){ // Skip hidden files
            size++;
        }
    }
    closedir(dir);

    files = malloc(sizeof(char*) * (size + 1));
    if (files == NULL) {
        perror("malloc");
        return NULL;
    }

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        for (int i = 0; files[i] != NULL; i++){
            free(files[i]);
        }
        free(files);
        return NULL;
    }

    int index = 0;
    while((entry = readdir(dir)) != NULL){
        if (entry->d_name[0] != '.' && entry->d_type == DT_REG){ // Skip hidden files
            
            files[index] = malloc(strlen(path) + strlen(entry->d_name) + 2);
            if (files[index] == NULL){
                closedir(dir);
                for (int i = 0; i < index; i++){
                    free(files[i]);
                }
                free(files);
                perror("malloc");
                return NULL;
            }
            snprintf(files[index], strlen(path) + strlen(entry->d_name) + 2, "%s/%s", path, entry->d_name);
            index++;
        }
    }
    closedir(dir);
    files[size] = NULL;
    return files;
}

char** format_for_loop_command(char** command, char* var_name, char* file_name){
    char** new_command = malloc(sizeof(char*) * (len_command(command) + 1));
    char new_var_name[strlen(var_name) + 2];
    sprintf(new_var_name, "$%s", var_name);
    if (new_command == NULL){
        perror("malloc");
        return NULL;
    }
    int i = 0;
    for (; i < len_command(command); i++){
        if (strcmp(command[i], new_var_name) == 0){
            new_command[i] = strdup(file_name);
            if (new_command[i] == NULL){
                perror("strdup");
                for (int j = 0; new_command[j] != NULL; j++){
                    free(new_command[j]);
                }
                free(new_command);
                return NULL;
            }
        } else {
            new_command[i] = strdup(command[i]);
            if (new_command[i] == NULL){
                perror("strdup");
                for (int j = 0; new_command[j] != NULL; j++){
                    free(new_command[j]);
                    }
                free(new_command);
                return NULL;
            }
        }
    }
    new_command[i] = NULL;
    return new_command;
}

//["for", "F", "in", "words2"], ["{"], ["ftype", "$F"], ["}"]
int run_for(char*** commands, int i, int last_val){
    if (len_command(commands[i]) != 4){
        perror("for loop must have 4 arguments");
        return 1;
    }
    char* var_name = commands[i][1];
    char* rep_path_name = commands[i][3];

    if (strcmp(commands[i+1][0], "{") != 0){
        perror("for loop must be followed by a block");
        return 1;
    }

    if (strcmp(commands[i+3][0], "}") != 0){
        perror("for loop must be closed with a }");
        return 1;
    }

    char** list_of_path_files = list_path_files(rep_path_name);
    if (list_of_path_files == NULL){
        perror("error in list_path_files");
        return 1;
    }

    int status = last_val;

    for (int j = 0; list_of_path_files[j] != NULL; j++){
        char** formated_command = format_for_loop_command(commands[i+2], var_name, list_of_path_files[j]);
        if (formated_command == NULL){
            perror("error in format_for_loop_command");
            return 1;
        }
        status = run_command(commands, formated_command, status);
        for (int k = 0; formated_command[k] != NULL; k++){
            free(formated_command[k]);
        }
        free(formated_command);
    }

    for (int j = 0; list_of_path_files[j] != NULL; j++){
        free(list_of_path_files[j]);
    }
    free(list_of_path_files);
    return status;

    
}

int run_command(char*** commands, char** command, int last_val){
    int status = last_val;
    char* command_name = command[0];
    // determine if the command is internal or external
    if (is_internal_command(command_name) == 1){
        // run the internal command
        // there must be a cleaner way to do this, this is just sad.
        if(strcmp(command_name, "exit") == 0){
            command_exit(commands, command, status);
        }else if (strcmp(command_name, "pwd") == 0){
            status = command_pwd(1);
        }else if (strcmp(command_name, "cd") == 0){
            status = command_cd(command);
        }else if (strcmp(command_name, "ftype") == 0){
            status = command_ftype(command);
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
            waitpid(pid, &status, 0);
            status = WEXITSTATUS(status);
       }
    }

    return status;
}

int run_commands(char*** commands, int last_val){
    // int stdin_copy = dup(STDIN_FILENO);
    // int stderr_cpy = dup(STDERR_FILENO);
    // int stdout_cpy = dup(STDOUT_FILENO);
    for (int i = 0; commands[i] != NULL; i++){
        if (commands[i][0] == NULL){
            continue;
        }
        else if (strcmp(commands[i][0], "for") == 0){
            last_val = run_for(commands, i, last_val);
            while(commands[i] != NULL && strcmp(commands[i][0], "}") != 0){
                i++;
            }
        }
        else{
            last_val = run_command(commands, commands[i], last_val);
        }
    }
    return last_val;
}