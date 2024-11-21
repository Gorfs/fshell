#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define PATH_MAX_LEN 4096

char* last_dir = NULL;

void cleanup() {
    if (last_dir != NULL) {
        free(last_dir);
        last_dir = NULL;
    }
}

int command_cd(char** command){

    if (command[1] != NULL && strcmp(command[1], "-") == 0){
        if (last_dir != NULL){
            if (chdir(last_dir) == 0){
                return 0;
            }else{
                write(STDERR_FILENO, "cd: No such file or directory\n", 31);
                return 1;
            }
        }else{
            write(STDERR_FILENO, "No previous directory\n", 23);
            return 1;
        }
    }

    char current_dir[PATH_MAX_LEN];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL){
        perror("getcwd() error");
        return 1;
    }

    if (last_dir != NULL){
        free(last_dir);
    }

    last_dir = strdup(current_dir);
    if (command[1] == NULL || strcmp(command[1], "~") == 0 || command[1][0] == '\0'){
        const char *home = getenv("HOME");
        if (chdir(home) == 0){
            return 0;
        }else{
            write(STDERR_FILENO, "cd: No such file or directory\n", 31);
            return 1;
        }
    }
    if (chdir(command[1]) == 0){
        return 0;
    }else{
        write(STDERR_FILENO, "cd: No such file or directory\n", 31);
        return 1;
    }
}
