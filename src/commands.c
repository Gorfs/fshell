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
#include <errno.h>
#include <fcntl.h>


// list of internal commands
char* internal_commands[] = {"exit", "pwd", "cd", "ftype", NULL};
// list of ignored delimiters when it comes to command execution
char* redirection_delimiters[] = {">", ">>", "<", "<<","|>", "|>>", "2>", "2>>", NULL};

int is_fd_valid(int fd) {
  int flags = fcntl(fd, F_GETFD);
  if (flags == -1 && errno == EBADF) {
    return -1;  // Invalid file descriptor
  } else {
    return 0;   // Valid file descriptor
  }
}


void print_file_descriptors(int** fdArray){
  for(int i = 0 ; fdArray[i] != NULL; i++){
    printf("fdArray[%d] = [%d, %d, %d]\n", i, fdArray[i][0], fdArray[i][1], fdArray[i][2]);
  }
}


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
      return 1;
    }
    i++;
  }
  return 0; 
}

/**
 * @brief check if a command is an internal command
 * @param command_name : the name of the command
 * @return 1 if the command is internal, 0 otherwise
 */
int is_internal_command(char* command_name){
  if(command_name == NULL){
    return 0;
  }
  return is_in_list(command_name, internal_commands);
}

/**
 * @brief check if a delimiter is not a redirection
 * @param delimiter : the delimiter to check
 * @return 1 if the delimiter is ignored, 0 otherwise
 */ 
int is_redirection_delimiter(char* delimiter){
  if (delimiter == NULL){
    return 0;
  }
  return is_in_list(delimiter, redirection_delimiters);
}

/**
 * @brief get quantity of commands in a list of commands
 * @param commands : the list of commands
 * @return the length of the total input
 */
int length_of_total_input(char*** commands){
  int count = 0;
  for(int i = 0 ; commands[i] != NULL; i++){
    if(is_redirection_delimiter(commands[i][0]) == 1){
      count--;
    }else if (is_delimiter(commands[i][0]) == 1){
      continue;
    }else{
      count++;
    }
  }
  return count;
}


/** 
 * @brief returns input and output file descriptors depending on delmiter
 * @param delmiter : string of the delimiter
 * @return int[2] : array of 2 integers, the first one is the input file descriptor, the second one is the output file descriptor
 * or null if an error occured
 */
int* handle_pipe(){
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


/**
 * @brief returns input and output file descriptors depending on delmiter
 * @param delimiter : string of the delimiter
 * @param file_name : string of the file name
 * @return int[2] : array of 2 integers, the first one is the input file descriptor, the second one is the output file descriptor
 * or null if an error occured
 */
int* handle_redirection(char* delimiter, char* file_name){
  // file descriptor for filename
  // default stdin
  int* fd = malloc(2 * sizeof(int));
  if(fd == NULL){
    perror("malloc");
    return NULL;
  }
  fd[0] = STDIN_FILENO;
  fd[1] = STDOUT_FILENO;
  // handle the possible redirection
  if(strcmp(delimiter, ">")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if (strcmp(delimiter, ">>")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if (strcmp(delimiter, "<")){
    fd[0] = open(file_name, O_RDONLY);
  }else if (strcmp(delimiter, "2>")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if (strcmp(delimiter, "2>>")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if(strcmp(delimiter, "<&")){
    fd[0] = open(file_name, O_RDONLY);
  }else if(strcmp(delimiter, "|>")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if(strcmp(delimiter, "|>>")){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if(strcmp(delimiter, "<>")){
    fd[0] = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
  } 
  return fd;
}


/**
 * @brief initializes an array of file descriptors to stdin and stdout
 * @param fdArray : the array of filedescriptors, null terminated
 * @return void
 */
int init_file_descriptors(int** fdArray, int length){
  if (fdArray == NULL){
    return 1;
  }
  for(int i = 0 ; i < length; i++){
    fdArray[i] = malloc(sizeof(int) * 3);
    if (fdArray[i] == NULL){
      goto error;
    }
    fdArray[i][0] = STDIN_FILENO;
    fdArray[i][1] = STDOUT_FILENO;
    fdArray[i][2] = STDERR_FILENO;
  }
  return 0;

error:
  for(int j = 0 ; fdArray[j]!= NULL; j++){
    free(fdArray[j]);
  }
  free(fdArray);
  return 1;
}


/**
 * @brief puts correct file descriptors for each command in commands
 * @param commands : the list of commands
 * @param fdArray : the array of file descriptors
 * @return 0 if successful, -1 otherwise
 */
int setup_fileDescriptors(char*** commands, int** fdArray){ 
  char* previous_delimiter = NULL;
  char* next_delimiter = NULL;
  int cmd_index = 0;
  // iterate through each command
  for(int i = 0; commands[i] != NULL; i++){
    // get the next delimiter
    if(commands[i+1] != NULL){
      next_delimiter = commands[i+1][0];
    }
    // if the command is a delimiter, we skip it after setting previous delim for the next iteration
    if(is_delimiter(commands[i][0])){
      previous_delimiter = commands[i][0];
      continue;
    }
    // if the previous delimiter is a redirection, we have skipped an input redirection, this is an edge case
    if(previous_delimiter != NULL && is_redirection_delimiter(previous_delimiter) == 1){
      int* fd = handle_redirection(previous_delimiter, commands[i][0]);
      fdArray[cmd_index -1][1] = fd[1];
      fdArray[cmd_index][0] = fd[0];
      cmd_index++;
    }
    
    // if the next delimiter is a pipe, we make a pipe and set the output of the command to the pipe as well as the input for the next command
    if(next_delimiter != NULL && strcmp(next_delimiter, "|") == 0){
      int* fd = handle_pipe();
      fdArray[cmd_index][1] = fd[1];
      fdArray[cmd_index+1][0] = fd[0];
      cmd_index++;
    }
    // if the next delimiter is a redirection, we set the input and output of the command to the correct file descriptors
    if(next_delimiter != NULL && is_redirection_delimiter(next_delimiter) == 1){
      char* file_name = commands[i+2][0];
      int* fd = handle_redirection(next_delimiter, file_name);
      if(fd == NULL){
        return -1;
      }
      fdArray[cmd_index][1] = fd[1];
      fdArray[cmd_index][0] = fd[0];
      // skip the next delimiter and the file name
      cmd_index++;
      i += 2;
    }
  }
  return 0;
}



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

  // here is the error
  if (output_fd != STDOUT_FILENO) {
    if (dup2(output_fd, STDOUT_FILENO) == -1){
      perror("dup2");
      return 1;
    }
  }
  if (input_fd != STDIN_FILENO) {
    if (dup2(input_fd, STDIN_FILENO) == -1){
      perror("dup2");
      return 1;
    }
  }
  int status = last_val;
  char* command_name = command[0];

  // Determine if the command is internal or external
  if (is_internal_command(command_name) == 1) {
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
      wait(&status);
      status = WEXITSTATUS(status);
    } else {
      // Fork error
      perror("fork");
      status = 1;
    }
  }
  if(output_fd != STDOUT_FILENO){
    close(output_fd);
  }
  if(input_fd != STDIN_FILENO){
    close(input_fd);
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
  // setup descriptor array
  int total_cmds = length_of_total_input(commands);
  int** command_fileDescriptors = malloc(sizeof(int*) * (total_cmds + 1));// +1 for the null terminator
  int cmd_index = -1; // set to -1 to start at 0 on first iteration
  command_fileDescriptors[total_cmds] = NULL; // null terminating array
  if(command_fileDescriptors == NULL){
    perror("malloc");
    return 1;
  }
  // init all descriptors to STDIN and OUT
  if(init_file_descriptors(command_fileDescriptors, total_cmds) != 0){
    perror("error alocation file_descriptor array");
    return 1;
  }
  // set up file descriptors
  if(setup_fileDescriptors(commands, command_fileDescriptors) != 0){
    perror("error setting up file descriptors");
    return 1;
  }
  
  for (int i = 0; commands[i] != NULL; i++){
    // close previous commands file discriptors
    cmd_index++;
    // base case
    if (commands[i][0] == NULL || is_delimiter(commands[i][0]) == 1){
      cmd_index--;
      continue;
    }else if (strcmp(commands[i][0], "for") == 0){
      last_val = run_for(commands, i, last_val);
      while(commands[i] != NULL && strcmp(commands[i][0], "}") != 0){
        i++;
      }
    }else if(commands[i + 1] != NULL && strcmp(commands[i+1][0], "|") == 0){ // if the next delimiter is a pipe
      // pipe, so we fork()
      if (fork() == 0){
        // child
        last_val = run_command(commands, commands[i], last_val, command_fileDescriptors[i][0], command_fileDescriptors[i][1]);
        exit(last_val);
      }else{
        // parent
        wait(NULL);
        // close file descriptors now that the child is done
        if(command_fileDescriptors[i][0] != STDIN_FILENO){
          close(command_fileDescriptors[i][0]);
        }
        if(command_fileDescriptors[i][1] != STDOUT_FILENO){
          close(command_fileDescriptors[i][1]);
        }
      }
    }else{
      // no pipe, so we run the command
      if(command_fileDescriptors[cmd_index] == NULL){
        printf("command_fileDescriptors[%d] is NULL\n", cmd_index);
      }
      if(commands[cmd_index] == NULL){
        printf("commands[%d] is NULL\n", cmd_index);
      }
      last_val = run_command(commands, commands[i], last_val, command_fileDescriptors[cmd_index][0], command_fileDescriptors[cmd_index][1]);
      // skip next delimiter if it's present
      if (commands[i+1] != NULL){
        cmd_index--;
        i++; 
      }
    }
  }

  // clean up potential pipes
  for(int i = 0 ; i < total_cmds; i++){
    if(command_fileDescriptors[i][0] != STDIN_FILENO){
      close(command_fileDescriptors[i][0]);
    }
    if(command_fileDescriptors[i][1] != STDOUT_FILENO){
      close(command_fileDescriptors[i][1]);
    }
    if(command_fileDescriptors[i][2] != STDERR_FILENO){
      close(command_fileDescriptors[i][2]);
    }
    free(command_fileDescriptors[i]);
  }
  free(command_fileDescriptors);
  
  reconnect_stdin_to_terminal(); // used to reconnect stdin to the terminal after pipes
  return last_val;
}
