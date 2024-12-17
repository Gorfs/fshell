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
        }
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

int replace_var_name_to_file_name(char*** commands, char* var_name, char* file_name) {
    if (!commands || !var_name || !file_name) {
        return 1; // Invalid input
    }

    for (int i = 0; commands[i]; i++) {
        for (int j = 0; commands[i][j]; j++) {
            char* str = commands[i][j];
            size_t var_len = strlen(var_name);
            size_t str_len = strlen(str);
            char* pos = str;
            
            while ((pos = strstr(pos, var_name)) != NULL) {
                size_t pos_index = pos - str;
                size_t new_len = str_len - var_len + strlen(file_name) + 1;
                char* new_str = malloc(new_len);

                if (!new_str) {
                    return 1; // Memory allocation failed
                }

                // Copy part before var_name
                strncpy(new_str, str, pos_index);
                new_str[pos_index] = '\0';

                // Append file_name and remaining part of the string
                strcat(new_str, file_name);
                strcat(new_str, pos + var_len);

                // Free old string and update to new string
                free(commands[i][j]);
                commands[i][j] = new_str;

                // Update string information for next search
                str_len = new_len - 1;
                str = new_str;
                pos = str + pos_index + strlen(file_name);
            }
        }
    }

    return 0; // Success
}

//["for", "F", "in", "words2"], ["{"], ["ftype", "$F"], ["}"]
int run_for(char*** commands, int i, int last_val){
    if (len_command(commands[i]) != 4){
        perror("for loop must have 4 arguments");
        return 1;
    }
    char* var_name = commands[i][1];
    char* final_var_name = malloc(strlen(var_name) + 2);
    if (final_var_name){
        strcpy(final_var_name, "$");
        strcat(final_var_name, var_name);
    }
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
        char*** tokens = tokenise_cmds(commands[i+2][0]);
        if (tokens == NULL){
            perror("error in tokenise_cmds");
            free(list_of_path_files);
            return 1;
        }
        int success = replace_var_name_to_file_name(tokens, final_var_name, list_of_path_files[j]);
        if (success != 0){
            perror("error in replace_var_name_to_file_name");
            free(list_of_path_files);
            free(tokens);
            return 1;
        }
        status = run_commands(tokens, status);
        free_tokens(tokens);
    }

    for (int j = 0; list_of_path_files[j] != NULL; j++){
        free(list_of_path_files[j]);
    }
    free(list_of_path_files);
    free(final_var_name);
    return status;
}

char*** create_blocks(char*** commands) {
    int cmd_len = 0; // length of the commands
    while (commands[cmd_len] != NULL) {
        cmd_len++;
    }

    char*** new_commands = malloc(sizeof(char**) * (cmd_len + 1)); // new array to store the commands
    if (new_commands == NULL) {
        perror("malloc");
        return NULL;
    }

    int new_i = 0; // index for the new array
    int j = 0; // index for the old array
    int n = 0; // count then number of open brackets, by adding 1 for each "{" and -1 for each "}"

    while (commands[j] != NULL) {
        // Adding the current block to the new commands array
        new_commands[new_i] = commands[j];
        new_i++;

        if (strcmp(commands[j][0], "{") == 0) { // First open brackets, start of the block
            int offset = j + 1;
            char* block = malloc(1 * sizeof(char)); // allocate memory for the block
            if (block == NULL) {
                perror("malloc");
                free(new_commands);
                return NULL;
            }
            block[0] = '\0'; // null terminate the block

            while (commands[offset] != NULL && (strcmp(commands[offset][0], "}") != 0 || n != 0)) {
                if (strcmp(commands[offset][0], "{") == 0) {
                    n++;
                } else if (strcmp(commands[offset][0], "}") == 0) {
                    n--;
                }

                // new size of the block
                size_t new_size = strlen(block) + strlen(commands[offset][0]) + 1; // +1 for the null terminator
                // reallocate memory for the block
                char* temp = realloc(block, new_size); // reallocate memory for the block
                if (temp == NULL) { // error handling
                    perror("realloc");
                    free(new_commands);
                    free(block);
                    return NULL;
                }
                block = temp;
                strcat(block, commands[offset][0]); // add the token to the block

                for (int k = 1; commands[offset][k] != NULL; k++) { // add the rest of the tokens
                    new_size = strlen(block) + strlen(commands[offset][k]) + 2; // +2 for the space and the null terminator
                    temp = realloc(block, new_size); // reallocate memory for the block
                    if (temp == NULL) { // error handling
                        perror("realloc");
                        free(new_commands);
                        free(block);
                        return NULL;
                    }
                    block = temp;

                    strcat(block, " ");
                    strcat(block, commands[offset][k]);
                }
                offset++;
            }
            new_commands[new_i] = malloc(sizeof(char*) * 2); // allocate memory for the block
            if (new_commands[new_i] == NULL) { // error handling
                perror("malloc");
                free(new_commands);
                free(block);
                return NULL;
            }
            char* temp = malloc(strlen(block) + 1); // allocate memory for the block
            if (temp == NULL) { // error handling
                perror("malloc");
                free(new_commands);
                free(block);
                free(temp);
                return NULL;
            }
            strcpy(temp, block); // copy the block to the new array
            new_commands[new_i][0] = temp; // add the block as a string to the new array

            new_commands[new_i][1] = NULL; // null terminate the block
            new_i++; // increment the index of the new array
            j = offset; // skip the block
            free(block); // free the block
        } else {
            j++;
        }
    }
    new_commands[cmd_len] = NULL; // null terminate the new array
    return new_commands;
}

int run_if(char*** commands, int i, int last_val) {
    // Checks if the "if" statement is followed by a block
    if (strcmp(commands[i+1][0], "{") != 0){
        perror("if statement must be followed by a block");
        return 1;
    }

    // Checks if the "if" statement is closed with a "}"
    if (strcmp(commands[i+3][0], "}") != 0){
        perror("if statement must be closed with a }");
        return 1;
    }

    // Get the status of the last command
    int status = last_val;

    // Init a new array to store the condition
    char ***if_condition = malloc(sizeof(char**) * 2);
    if (if_condition == NULL) {
        perror("malloc");
        return 1;
    }
    // Set the condition to the command after the "if" statement
    if_condition[0] = commands[i] + 1;
    if_condition[1] = NULL;

    // Run the condition
    status = run_commands(if_condition, status);

    if (status == 0) { // if the status is 0, then the condition is true
        // Init a new array to store the block of commands if the condition is true
        char ***if_then = tokenise_cmds(commands[i+2][0]); // Run the block of commands if the condition is true
        status = run_commands(if_then, status);
        free_tokens(if_then);

    } else if (commands[i+4] != NULL && strcmp(commands[i+4][0], "else") == 0) { // if the status is not 0, then the condition is false
        // Checks if the "else" statement is followed by a block
        if (strcmp(commands[i+5][0], "{") != 0){
            perror("else statement must be followed by a block");
            free(if_condition);
            return 1;
        }

        // Checks if the "else" statement is closed with a "}"
        if (strcmp(commands[i+7][0], "}") != 0){
            perror("else statement must be closed with a }");
            free(if_condition);
            return 1;
        }

        // Init a new array to store the block of commands if the condition is false
        char ***if_else = tokenise_cmds(commands[i+6][0]);
        // Run the block of commands after the "else" statement
        status = run_commands(if_else, status);
        free_tokens(if_else);
    } else {
        status = 0; // if there is no "else" statement, the status is 0
    }

    free(if_condition);
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
    if (commands == NULL) {
        perror("error creating blocks");
        return 1;
    }

    for (int i = 0; commands[i] != NULL; i++){
        if (commands[i][0] == NULL){
            continue;
        }
        else if (strcmp(commands[i][0], "for") == 0) {
            last_val = run_for(commands, i, last_val);
            while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) {
                i++;
            }
        } else if (strcmp(commands[i][0], "if") == 0) {
            last_val = run_if(commands, i, last_val);
            while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) {
                i++;
            }
            // check if there is an else statement
            if (commands[i + 1] != NULL && strcmp(commands[i + 1][0], "else") == 0) {
                i++;
                while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) {
                    i++;
                }
            }
        } else {
            last_val = run_command(commands, commands[i], last_val);
        }
    }
    return last_val;
}