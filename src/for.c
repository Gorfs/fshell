#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
// include our own files
#include <commands.h>
#include <exit.h>
#include <for.h>
#include <tokenisation.h>

/**
 * @brief get the amount of tokens of a command
 * @param tokens : the command
 * @return the length of the command
 */
int len_command(char** command){
    int i = 0;
    while(command[i]){
        i++;
    }
    return i;
}

/**
 * @brief Function that checks if the extension is the right one.
 * @param file_path path of the file
 * @param extension the extension to check
 * @return 0 if successful, 1 otherwise
 */
int test_extension(char* file_path, char* extension){
    char* dot = strrchr(file_path, '.');
    if (dot == NULL || strcmp(dot + 1, extension) != 0) return 0;
    else return 1;
}


/**
 * @brief Function that counts the number of files in a directory.
 * @param path         the path of the directory
 * @param hidden_files 1 if we want to count hidden files, 0 otherwise
 * @param recursive    1 if we want to count files recursively, 0 otherwise
 * @param extension    the extension of the files we want to count
 * @param type         the type of the files we want to count
 * @return a list of strings containing the path of the files in the directory
 */
int count_files_recursive(char* path, int hidden_files, int recursive, char* extension, char type) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(path);
    if (dir == NULL) { // Error handling
        perror("opendir");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((entry->d_name[0] != '.' || (hidden_files == 1 && entry->d_name[0] == '.')) &&
                strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

            int path_len = strlen(path);
            int name_len = strlen(entry->d_name);
            int full_path_len = path_len + 1 + name_len + 1;

            char *full_path = malloc(full_path_len);
            if (full_path == NULL) { // Error handling
                perror("malloc");
                closedir(dir);
                return count;
            }

            snprintf(full_path, full_path_len, "%s/%s", path, entry->d_name);

            struct stat st;
            if (lstat(full_path, &st) == 0) {
                int include;
                switch (type) {
                    case 'f':
                        include = S_ISREG(st.st_mode);
                        break;
                    case 'd':
                        include = S_ISDIR(st.st_mode);
                        break;
                    case 'l':
                        include = S_ISLNK(st.st_mode);
                        break;
                    case 'p':
                        include = S_ISFIFO(st.st_mode);
                        break;
                    default:
                        include = 1;
                        break;
                }

                int is_extension = 0;
                if (extension != NULL) is_extension = test_extension(full_path, extension);

                if (include || (recursive && S_ISDIR(st.st_mode))) {
                    if (S_ISDIR(st.st_mode) && recursive)
                        count += count_files_recursive(full_path, hidden_files, recursive, extension, type);
                    if ((extension == NULL && include) || (extension != NULL && is_extension == 1 && include))
                        count++;
                }
            }
            free(full_path);
        }
    }
    closedir(dir);
    return count;
}


/**
 * @brief Function that lists the files in a directory.
 * @param path         the path of the directory
 * @param hidden_files 1 if we want to list hidden files, 0 otherwise
 * @param recursive    1 if we want to list files recursively, 0 otherwise
 * @param extension    the extension of the files we want to list
 * @param type         the type of the files we want to list
 * @return a list of strings containing the path of the files in the directory
 */
char** list_path_files(char* path, int hidden_files, int recursive, char* extension, char type) {
    int size = count_files_recursive(path, hidden_files, recursive, extension, type);
    char** files = malloc(sizeof(char*) * (size + 1));
    if (files == NULL) { // Error handling
        perror("malloc");
        return NULL;
    }

    DIR *dir;
    struct dirent *entry;
    int index = 0;

    dir = opendir(path);
    if (dir == NULL) { // Error handling
        perror("opendir");
        free(files);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((entry->d_name[0] != '.' || (hidden_files == 1 && entry->d_name[0] == '.')) &&
                strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

            int path_len = strlen(path);
            int name_len = strlen(entry->d_name);
            int full_path_len = path_len + 1 + name_len + 1;

            char *full_path = malloc(full_path_len);
            if (full_path == NULL) { // Error handling
                perror("malloc");
                closedir(dir);
                for (int i = 0; i < index; i++) {
                    free(files[i]);
                }
                free(files);
                return NULL;
            }

            snprintf(full_path, full_path_len, "%s/%s", path, entry->d_name);
            int is_extension = 0;
            if (extension != NULL){
                is_extension = test_extension(full_path, extension);
            }
            struct stat st;
            if (lstat(full_path, &st) == 0) {
                int include;
                switch (type) {
                    case 'f':
                        include = S_ISREG(st.st_mode);
                        break;
                    case 'd':
                        include = S_ISDIR(st.st_mode);
                        break;
                    case 'l':
                        include = S_ISLNK(st.st_mode);
                        break;
                    case 'p':
                        include = S_ISFIFO(st.st_mode);
                        break;
                    default:
                        include = 1;
                        break;
                }

                if (include || (recursive && S_ISDIR(st.st_mode))) {
                    if (S_ISDIR(st.st_mode) && recursive) {
                        if (extension == NULL && include){
                            files[index] = strdup(full_path);
                            if (files[index] == NULL) {
                                perror("strdup");
                                closedir(dir);
                                for (int j = 0; j < index; j++) {
                                    free(files[j]);
                                }
                                free(files);
                                return NULL;
                            }
                            index++;
                        }
                        char** sub_files = list_path_files(full_path, hidden_files, recursive, extension, type);
                        if (sub_files != NULL) {
                            for (int i = 0; sub_files[i] != NULL; i++, index++) {
                                files[index] = strdup(sub_files[i]);
                                if (files[index] == NULL) {
                                    perror("strdup");
                                    closedir(dir);
                                    for (int j = 0; j < index; j++) {
                                        free(files[j]);
                                    }
                                    free(files);
                                    free(sub_files);
                                    return NULL;
                                }
                                free(sub_files[i]);
                            }
                            free(sub_files);
                        }
                    } else {
                        if (extension == NULL || is_extension == 1) {
                            files[index] = strdup(full_path);
                            if (files[index] == NULL) {
                                perror("strdup");
                                closedir(dir);
                                for (int j = 0; j < index; j++) {
                                    free(files[j]);
                                }
                                free(files);
                                return NULL;
                            }
                            if (extension != NULL) {
                                char *filename = files[index];
                                char *dot = strrchr(filename, '.');
                                if (dot != NULL && dot != filename && *(dot - 1) != '/') {
                                    *dot = '\0';
                                }
                            }
                            index++;
                        }
                    }
                }
            }
            free(full_path);
        }
    }
    closedir(dir);
    files[size] = NULL;
    return files;
}


/**
 * @brief Function that replaces a variable name by a file name in a list of commands.
 * @param commands  the list of commands
 * @param var_name  the variable name to replace
 * @param file_name the file name to replace with
 * @return
 */
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

                if (!new_str) return 1; // Memory allocation failed

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


/**
 * @brief Function that runs the for loop.
 * @param commands he list of commands
 * @param i        the index of the for loop
 * @param last_val the value of the last command
 * @return the value of the last command
 */
int run_for(char*** commands, int i, int last_val){
    int cmd_first_token_size = 0; // length of the first token
    for (int j = 0; commands[i][j] != NULL; j++) {
        cmd_first_token_size++;
    }
    if (cmd_first_token_size < 4) {
        perror("syntax error");
        return 2;
    }

    if (strcmp(commands[i][0], "for") != 0 || strcmp(commands[i][2], "in") != 0){
        perror("syntax error");
        return 2;
    }

    char* var_name = commands[i][1];
    char* final_var_name = malloc(strlen(var_name) + 2);
    if (final_var_name){
        strcpy(final_var_name, "$");
        strcat(final_var_name, var_name);
    }
    char* rep_path_name = commands[i][3];

    if (strcmp(commands[i+1][0], "{") != 0) {
        perror("for loop must be followed by a block");
        return 2;
    }

    if (strcmp(commands[i+3][0], "}") != 0){
        perror("for loop must be closed with a }");
        return 2;
    }

    int hidden_files = 0;
    int recursive = 0;
    char* extension = NULL;
    char type = ' ';
    int num_processes = -1;

    for (int j = 4; commands[i][j] != NULL; j++) {
        if (strcmp(commands[i][j], "-A") == 0) {
            hidden_files = 1;
        } else if (strcmp(commands[i][j], "-r") == 0) {
            recursive = 1;
        } else if (strcmp(commands[i][j], "-e") == 0) {
            if (commands[i][j+1] != NULL) {
                extension = commands[i][j+1];
                j++;  // Skip the next element as it's the extension
            }
        } else if (strcmp(commands[i][j], "-t") == 0) {
            if (commands[i][j+1] != NULL) {
                if (strcmp(commands[i][j+1], "f") == 0) {
                    type = 'f';
                } else if (strcmp(commands[i][j+1], "d") == 0) {
                    type = 'd';
                } else if (strcmp(commands[i][j+1], "l") == 0) {
                    type = 'l';
                } else if (strcmp(commands[i][j+1], "p") == 0) {
                    type = 'p';
                }
                j++;  // Skip the next element as it's the type
            }
        } else if (strcmp(commands[i][j], "-p") == 0) {
            if (commands[i][j+1] != NULL) {
                num_processes = atoi(commands[i][j+1]);
                j++;  // Skip the next element as it's the number
            }
        }
    }

    char** list_of_path_files = list_path_files(rep_path_name, hidden_files, recursive, extension, type);
    if (list_of_path_files == NULL){
        perror("error in list_path_files");
        return 1;
    }

    int file_index = 0;
    pid_t pids[num_processes];
    int process_status[num_processes];
    int status = last_val;
    int max_status = 0;

    if (num_processes > 1){
        while (list_of_path_files[file_index] != NULL){
            for(int p = 0; p < num_processes && list_of_path_files[file_index] != NULL; p++){
                pids[p] = fork();
                if (pids[p] < 0){
                    perror("error in fork");
                    for (int k = 0; k < p; k++){
                        waitpid(pids[k], &process_status[k],0);
                    }
                    free(list_of_path_files);
                    return 1;
                }
                else if(pids[p] == 0){
                    char*** tokens = tokenise_cmds(commands[i+2][0]);
                    if (tokens == NULL){
                        perror("error in tokenise_cmds");
                        free(list_of_path_files);
                        exit(1);
                    }
                    int success = replace_var_name_to_file_name(tokens, final_var_name, list_of_path_files[file_index]);
                    if (success != 0){
                        perror("error in replace_var_name_to_file_name");
                        free_tokens(tokens);
                        exit(1);
                    }
                    int child_status = run_commands(tokens, status);
                    free_tokens(tokens);
                    exit(child_status);
                }
                file_index++;
            }

            for (int p = 0; p < num_processes; p++){
                if (pids[p] > 0){
                    waitpid(pids[p],&process_status[p],0);
                    if(WIFEXITED(process_status[p])){
                        int child_status = WEXITSTATUS(process_status[p]);
                        if (child_status == -1){
                            max_status = child_status;
                            break;
                        }
                        else if(child_status > max_status){
                            max_status = child_status;
                        }
                    }
                }
            }

            if(max_status == 1){
                break;
            }
        }
    }
    else{
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
            if (status == -1) {
                max_status = status;
                break; // If SIGINT is caught, break the loop
            } else if (status > max_status) {
                max_status = status;
            }
            free_tokens(tokens);
        }
    }

    for (int j = 0; list_of_path_files[j] != NULL; j++) {
        free(list_of_path_files[j]);
    }
    free(list_of_path_files);
    free(final_var_name);
    return max_status;
}
