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
#include <fcntl.h>

// list of internal commands
char* internal_commands[] = {"exit", "pwd", "cd", "ftype", NULL};
// list of ignored delimiters when it comes to command execution
char* ignored_delimiters[] = {";", "{", "}", NULL};

/**
 * @brief reconnect stdin to the terminal
 * @return 0 if successful, -1 otherwise
 */
int reconnect_stdin_to_terminal() {
    int tty_fd = open("/dev/tty", O_RDWR);
    if (tty_fd == -1) {
        perror("open(/dev/tty)");
        return -1;
    }

    if (dup2(tty_fd, STDIN_FILENO) == -1) {
        perror("dup2(tty_fd, STDIN_FILENO)");
        close(tty_fd);
        return -1;
    }

    close(tty_fd);
    return 0;
}

/**
 * @brief check if a string is in a list of strings, supposes list is null terminated and length is >= 1 
 * @param str : the string to check
 * @param list : the list of strings
 * @return 1 if the string is in the list, 0 otherwise
 */
int is_in_list(char* str, char** list){
  int i = 0 ;
  while(list[i] != NULL){
    if(strcmp(str, list[i]) == 0){
      return 0;
    }
    i++;
  }
  return 1;
}

/**
 * @brief check if a command is an internal command
 * @param command_name : the name of the command
 * @return 1 if the command is internal, 0 otherwise
 */
int is_internal_command(char* command_name){
  return is_in_list(command_name, internal_commands);
}

/**
 * @brief check if a delimiter is ignored
 * @param delimiter : the delimiter to check
 * @return 1 if the delimiter is ignored, 0 otherwise
 */ 
int is_ignored_delimiter(char* delimiter){
  if (delimiter == NULL){
    return 0;
  }
  return is_in_list(delimiter, ignored_delimiters);
}

/**
 * @brief get the amount of tokens of a command
 * @param tokens : the command
 * @return the length of the command
 */
int len_command(char** tokens){
    int i = 0;
    while(tokens[i]){
        i++;
    }
    return i;
}

/**
 * @brief list all the files in a directory
 * @param path : the path of the directory
 * @return a list of strings containing the path of the files in the directory
 */
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

/**
 * @brief format a command for a for loop
 * @param command : the command to format
 * @param var_name : the name of the variable
 * @param file_name : the name of the file
 * @return the formatted command
 */
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
/**
 * @brief run a for loop
 *  @param commands : the list of commands
 *  @param i : the index of the for loop
 *  @param last_val : the value of the last command
 *  @return the value of the last command
 */ 
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
        status = run_command(commands, formated_command, status, STDIN_FILENO, STDOUT_FILENO );
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

/** 
 * @brief returns input and output file descriptors depending on delmiter
 * @param delmiter : string of the delimiter
 * @return int[2] : array of 2 integers, the first one is the input file descriptor, the second one is the output file descriptor
 * or null if an error occured
 */
int* handle_redirection(char* delimiter){
  if(strcmp(delimiter, "|") == 0){
    // pipe
    int* fd = malloc(sizeof(int) * 2);
    if(fd == NULL){
      perror("malloc");
      return NULL;
    }
    if(pipe(fd) == -1){
      perror("pipe");
      return NULL;
    }
    return fd;
  }
  // default case, if no redirection is found
  return NULL;
}


// Function to run a single command, handling redirection and internal commands
/**
 * @brief run a single command
 * @param commands : the list of commands (only required for command_exit())
 * @param command : the command to run
 * @param last_val : the value of the last command
 * @param input_fd : the input file descriptor
 * @param output_fd : the output file descriptor
 * @return the value of the last command
 */
int run_command(char*** commands, char** command, int last_val, int input_fd, int output_fd) {
    // Redirection for input and output
    if (input_fd != STDIN_FILENO) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != STDOUT_FILENO) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    int status = last_val;
    char* command_name = command[0];

    // Determine if the command is internal or external
    if (is_internal_command(command_name) == 0) {
        // Run the internal command
        if (strcmp(command_name, "exit") == 0) {
            command_exit(commands, command, status);
        } else if (strcmp(command_name, "pwd") == 0) {
            status = command_pwd(STDOUT_FILENO);
        } else if (strcmp(command_name, "cd") == 0) {
            status = command_cd(command);
        } else if (strcmp(command_name, "ftype") == 0) {
            status = command_ftype(command);
        } else {
            // Error handling
            fprintf(stderr, "error running internal command: %s\n", command_name);
            status = 1;
        }
    } else {
        // Make a new process for external commands
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execvp(command_name, command);
            perror("error executing the command");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            waitpid(pid, &status, 0);
            status = WEXITSTATUS(status);
        } else {
            // Fork error
            perror("fork");
            status = 1;
        }
    }

    return status;
}

/**
 * @brief run a list of commands
 * @param commands : the list of commands
 * @param last_val : the value of the last command
 * @return the value of the last command
 */
int run_commands(char*** commands, int last_val){
  int input_fd = STDIN_FILENO;
  int output_fd = STDOUT_FILENO;
  for (int i = 0; commands[i] != NULL; i++){
    // set the next delimiter
    char* next_delimiter = NULL;
    if(commands[i+1] != NULL){
      next_delimiter = commands[i+1][0];
    }
    // base case
    if (commands[i][0] == NULL){
      continue;

    }else if (strcmp(commands[i][0], "for") == 0){
      last_val = run_for(commands, i, last_val);
      while(commands[i] != NULL && strcmp(commands[i][0], "}") != 0){
        i++;
      }

    }else if (is_ignored_delimiter(next_delimiter) == 1){ // if the next delimiter is not ignored
      int* fd = handle_redirection(next_delimiter); 
      int pid = fork();
      if (pid == 0){
        // child process
        output_fd = fd[1];
        last_val = run_command(commands, commands[i], last_val, input_fd, output_fd);
        // if the input is a pipe, we close it
        if (input_fd != STDIN_FILENO){
          close(input_fd);
        }
        // we are dealing with a delimiter for sure, no need to check if it's a pipe
        close(fd[1]);
        // close the child process
        exit(last_val);

      }else{
        // parent process
        // we never write to the pipe so we can close it directly
        close(fd[1]);  // this line is important, otherwise the the shell will hang
        if(input_fd != STDIN_FILENO){ // close previous pipe
          close(input_fd);
        }
        input_fd = fd[0]; // the input of the parent (and the subsequent command) is the output from the command
        // do not modify the output of the parent
        int result;
        waitpid(pid , &result, 0);
        last_val = WEXITSTATUS(result);
        i++; // skip next delimiter
      }

    }else{
      // default case for command running
      last_val = run_command(commands, commands[i], last_val, input_fd, output_fd);
      // close input pipe if it exists
      if(input_fd != STDIN_FILENO){
        close(input_fd);
      }
      // reset intput for next command
      input_fd = STDIN_FILENO;
      // skip next delimiter if it's present
      if (commands[i+1] != NULL){
        i++; 
      }
    }
  }
  // clean up potential pipes
  if(input_fd != STDIN_FILENO){
    close(input_fd);
  }
  if(output_fd != STDOUT_FILENO){
    close(output_fd);
  }
  reconnect_stdin_to_terminal(); // used to reconnect stdin to the terminal after pipes
  return last_val;
}
