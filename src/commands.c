#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h> // temporary to use printf for debugging
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
// include our own files
#include <cd.h>
#include <commands.h>
#include <exit.h>
#include <for.h>
#include <ftype.h>
#include <tokenisation.h>
#include <pwd.h>
#include <if.h>


// list of internal commands
char* internal_commands[] = {"exit", "pwd", "cd", "ftype", NULL};
// list of ignored delimiters when it comes to command execution
char* redirection_delimiters[] = {">", ">>", "<", "<<","|>",">|", "|>>", "2>", "2>>","2>|", NULL};


void command_signal(int sig) {
    exit(sig);
}


/**
 * @brief Function that checks if the file descriptor is valid.
 * @param fd the file descriptor to check
 * @return 0 if the file descriptor is valid, -1 otherwise
 */
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
 * @brief Function that checks if a string is in a list of strings.
 * @param str  the string to check, supposed to be not NULL
 * @param list the list of strings
 * @return 1 if the string is in the list, 0 otherwise
 */
int is_in_list(char* str, char** list) {
    int index = 0;
    while(list[index] != NULL) {
        if (strcmp(str, list[index]) == 0) return 1;
        index++;
    }
    return 0;
}


/**
 * @brief Function that checks if a command is an internal command.
 * @param command_name the name of the command
 * @return 1 if the command is internal, 0 otherwise
 */
int is_internal_command(char* command_name) {
    if (command_name == NULL) return 0; // if the command is NULL, it is not an internal command
    return is_in_list(command_name, internal_commands);
}


/**
 * @brief Function that checks if a string is a delimiter.
 * @param delimiter the delimiter to check
 * @return 1 if the delimiter is ignored, 0 otherwise
 */ 
int is_redirection_delimiter(char* delimiter) {
    if (delimiter == NULL) return 0; // if the delimiter is NULL, it is not a delimiter
    return is_in_list(delimiter, redirection_delimiters);
}


/**
 * @brief Function that returns the length of a command.
 * @param commands array of array of strings representing the commands
 * @return the length of the total input
 */
int length_of_total_input(char*** commands) {
    int count = 0;
    for(int i = 0; commands[i] != NULL; i++) {
        if (is_redirection_delimiter(commands[i][0]) == 1) count--;
        else if (is_delimiter(commands[i][0]) == 1) continue;
        else count++;
    }
    return count;
}


/**
 * @brief Function that create a file and its directories if they don't exist.
 * @param path the path of the file
 * @return 0 if successful, -1 if an error occurred
 */
int create_directory_file(char* path) {
    char* path_copy = strdup(path);
    if(path_copy == NULL) {
        perror("strdup");
        return -1;
    }
    char* token = strtok(path_copy, "/");
    char* token2 = strtok(NULL, "/");
    char* current_path = malloc(sizeof(char) * strlen(path) + 1);
    strcpy(current_path, token);
    while(token2 != NULL) {
        if(access(current_path, F_OK) == -1) { // try to access the directory
            if(mkdir(current_path, 0777) == -1) { // try to create the directory
                perror("mkdir");
                return -1;
            }
        }
        // the directory now exists, we can move on to the next one
        current_path = strcat(current_path, "/");
        current_path = strcat(current_path, token2);
        token2 = strtok(NULL, "/");
    }
    free(path_copy);
    free(current_path);
    return 0;
}


/** 
 * @brief Function that handles the pipe.
 * @return an array of 2 integers, the first one is the input file descriptor,
 *         the second one is the output file descriptor
 */
int* handle_pipe() {
    int* fd = malloc(sizeof(int) * 2); // 2 file descriptors
    if(fd == NULL) { // error handling
        perror("malloc");
        return NULL;
    }
    if(pipe(fd) == -1) { // error handling
        perror("pipe");
        return NULL;
    }
    return fd;
}


/**
 * @brief Function that handles the redirection.
 * @param delimiter string of the delimiter
 * @param filepath string representing th
 * @return an array of 3 integers, the first one is the input file descriptor,
 */
int* handle_redirection(char* delimiter, char* filepath){
    int* fd = malloc(3 * sizeof(int)); // 3 file descriptors, one for each standard stream
    if (fd == NULL) { // error handling
        perror("malloc");
        return NULL;
    }
    fd[0] = STDIN_FILENO; // standard input
    fd[1] = STDOUT_FILENO; // standard output
    fd[2] = STDERR_FILENO; // standard error
    // handle the possible redirection
    if (strcmp(delimiter, ">") == 0) {
        // check if the file already exists
        if (access(filepath, F_OK) != -1) {
            write(STDERR_FILENO, "pipeline_run:␣File␣exists\n", 30);
            fd[1] = -1;
        } else {
            if(create_directory_file(filepath) == -1) { // try to create the file
                return NULL;
            }
            fd[1] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
    } else if (strcmp(delimiter, ">>") == 0) { // append to the file
        fd[1] = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    } else if (strcmp(delimiter, "<") == 0) { // input redirection
        fd[0] = open(filepath, O_RDONLY);
    } else if (strcmp(delimiter, "2>") == 0) { // standard error redirection
        if(access(filepath, F_OK) != -1) { // check if the file already exists
            write(STDERR_FILENO, "pipeline_run:␣File␣exists", 26);
            fd[2] = -1;
        } else {
            if(create_directory_file(filepath) == -1) { // try to create the file
                return NULL;
            }
            fd[2] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
    } else if(strcmp(delimiter, "2>|") == 0) { // standard error redirection with overwrite
        if (create_directory_file(filepath) == -1) { // try to create the file
            return NULL;
        }
        fd[2] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    } else if (strcmp(delimiter, "2>>") == 0) { // standard error redirection with append
        fd[2] = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    } else if (strcmp(delimiter, "<&") == 0) { // input redirection
        fd[0] = open(filepath, O_RDONLY);
    } else if (strcmp(delimiter, "|>") == 0) { // output redirection with overwrite
        fd[1] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    } else if (strcmp(delimiter, "|>>") == 0) { // output redirection with append
        fd[1] = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    } else if (strcmp(delimiter, ">|") == 0) { // output redirection with overwrite
        if (create_directory_file(filepath) == -1) { // try to create the file
            return NULL;
        }
        fd[1] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    } else if (strcmp(delimiter, "<>") == 0) {
        // pretty sure this one is not an actual redirection
        fd[0] = open(filepath, O_RDWR | O_CREAT | O_TRUNC, 0666);
    }
    if (fd[0] == -1 || fd[1] == -1 || fd[2] == -1) {
        perror("open");
        goto error;
    }
    return fd;

    error:
        free(fd);
        return NULL;
}


/**
 * @brief Function that initializes an array of file descriptors to stdin and stdout.
 * @param fdArray the array of file descriptors, null terminated
 * @return 0 if successful, -1 otherwise
 */
int init_file_descriptors(int** fdArray, int length) {
    if (fdArray == NULL) {
        return 1;
    }
    for(int i = 0 ; i < length; i++){
        fdArray[i] = malloc(sizeof(int) * 3);
        if (fdArray[i] == NULL){
            for(int j = 0 ; fdArray[j]!= NULL; j++){
                free(fdArray[j]);
            }
            return -1;
        }
        fdArray[i][0] = STDIN_FILENO;
        fdArray[i][1] = STDOUT_FILENO;
        fdArray[i][2] = STDERR_FILENO;
    }
    return 0;
}


/**
 * @brief Function that puts correct file descriptors for each command in commands
 * @param commands : the list of commands
 * @param fdArray : the array of file descriptors
 * @return 0 if successful, -1 otherwise
 */
int setup_file_descriptors(char*** commands, int** fdArray) {
    char* previous_delimiter = NULL;
    char* next_delimiter = NULL;
    int cmd_index = 0;
    // iterate through each command
    for (int i = 0; commands[i] != NULL; i++) {
        // get the next delimiter
        if (commands[i+1] != NULL) {
            next_delimiter = commands[i+1][0];
        }
        // if the command is a delimiter, we skip it after setting previous delim for the next iteration
        if (is_delimiter(commands[i][0])) {
            previous_delimiter = commands[i][0];
            continue;
        }
        // if the previous delimiter is a redirection, we have skipped an input redirection, this is an edge case
        if (previous_delimiter != NULL && is_redirection_delimiter(previous_delimiter) == 1) {
            cmd_index--;
            int* fd = handle_redirection(previous_delimiter, commands[i][0]);
            if (fd == NULL) { // error handling
                return -1;
            }
            if (fdArray[cmd_index][2] == STDERR_FILENO) {
                fdArray[cmd_index][2] = fd[2];
            }
            if (fdArray[cmd_index][0] == STDIN_FILENO) {
                fdArray[cmd_index][0] = fd[0];
            }
            if (fdArray[cmd_index][1] == STDOUT_FILENO) {
                fdArray[cmd_index][1] = fd[1];
            }
            free(fd);// free the pointer to the fd
        }
    
        // if the next delimiter is a pipe, we make a pipe and set the output of the
        // command to the pipe as well as the input for the next command
        if (next_delimiter != NULL && strcmp(next_delimiter, "|") == 0) {
            int* fd = handle_pipe();
            fdArray[cmd_index][1] = fd[1];
            fdArray[cmd_index+1][0] = fd[0];
            free(fd); // free the pointer
            cmd_index++;
        }

        // if the next delimiter is a redirection, we set the input and output of the
        // command to the correct file descriptors
        if (next_delimiter != NULL && is_redirection_delimiter(next_delimiter) == 1) {
            char* file_name = commands[i+2][0];
            int* fd = handle_redirection(next_delimiter, file_name);
            if (fd == NULL) return -1;
            if (fdArray[cmd_index][2] == STDERR_FILENO) {
                fdArray[cmd_index][2] = fd[2];
            }
            if(fdArray[cmd_index][0] == STDIN_FILENO){
                fdArray[cmd_index][0] = fd[0];
            }
            if(fdArray[cmd_index][1] == STDOUT_FILENO){
                fdArray[cmd_index][1] = fd[1];
            }
            free(fd);// free the pointer
              // skip the next delimiter and the file name
            cmd_index++;
            i += 2;
        }
    }
    return 0;
}


/**
 * @brief Function that runs a single command.
 * @param commands  the list of commands (only required for command_exit())
 * @param command   the command to run
 * @param last_val  the value of the last command
 * @param fd an array of file descriptors for input, output and error
 * @return the value of the last command
 */
int run_command(char*** commands, char** command, int last_val, int* fd) {
    int input_fd = fd[0];
    int output_fd = fd[1];
    int error_fd = fd[2];
    // check if the output file descriptor is valid
    if (output_fd == -1) return 1;
    if (error_fd == -1) return 1;
    if (input_fd == -1) return 1;
    if (is_fd_valid(output_fd) == -1) {
        perror("output file descriptor is invalid");
        return 1;
    }
    // check if the input file descriptor is valid
    if (is_fd_valid(input_fd) == -1) {
        perror("input file descriptor is invalid");
        return 1;
    }

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
    if(error_fd != STDERR_FILENO){
        if(dup2(error_fd, STDERR_FILENO) == -1){
            perror("dup2");
            return 1;
        }
    }
  
    int status = last_val;
    char* command_name = command[0];

    // Determine if the command is internal or external
    if (is_internal_command(command_name) == 1) { // Run the internal command
        if (strcmp(command_name, "exit") == 0) {
            status = command_exit(commands, command, status);
        } else if (strcmp(command_name, "pwd") == 0) {
            status = command_pwd(command);
        } else if (strcmp(command_name, "cd") == 0) {
            status = command_cd(command);
        } else if (strcmp(command_name, "ftype") == 0) {
            status = command_ftype(command);
        } else { // Error handling
            fprintf(stderr, "error running internal command: %s\n", command_name);
            status = 1;
        }
    } else { // Make a new process for external commands
        pid_t pid = fork();
        if (pid == 0) { // Child process
            struct sigaction sa;
            sa.sa_handler = command_signal;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, NULL);
            sa.sa_handler = SIG_DFL;
            sigaction(SIGTERM, &sa, NULL);

            execvp(command_name, command);
            perror("error executing the command");
            exit(1);
        } else if (pid > 0) { // Parent process
            waitpid(pid, &status, 0);
            // check if the process was terminated by a signal
            if (WIFSIGNALED(status)) {
                if (WTERMSIG(status) == SIGINT) status = -1; // to differentiate from value 255 and print SIG in the prompt
                else if (WTERMSIG(status) == SIGTERM) status = -2;
            } else status = WEXITSTATUS(status);
        } else { // Error handling
            perror("fork");
            status = 1;
        }
    }
    if (output_fd != STDOUT_FILENO) close(output_fd);
    if (input_fd != STDIN_FILENO) close(input_fd);

    return status;
}


/**
 * @brief run a list of commands
 * @param commands : the list of commands
 * @param last_val : the value of the last command
 * @return the value of the last command
 */
int run_commands(char*** commands, int last_val){
    if (last_val == -1) return last_val; // if the last command was interrupted by a signal
    int dup_stdin = dup(STDIN_FILENO);
    int dup_stdout = dup(STDOUT_FILENO);
    int dup_stderr = dup(STDERR_FILENO);
    // print_tokenised_cmds(commands);
    // setup descriptor array
    int total_cmds = length_of_total_input(commands);
    int** cmd_fd = malloc(sizeof(int*) * (total_cmds + 1));// +1 for the null terminator
    if (cmd_fd == NULL) { // Error handling
        perror("malloc");
        return 1;
    }
    cmd_fd[total_cmds] = NULL; // null terminating array
    int cmd_index = 0;

    // init all descriptors to STDIN and OUT
    if (init_file_descriptors(cmd_fd, total_cmds) != 0) {
        perror("error alocation file_descriptor array");
        return 1;
    }
    // set up file descriptors
    if (setup_file_descriptors(commands, cmd_fd) != 0) {
        perror("error setting up file descriptors");
        return 1;
    }
  
    for (int i = 0; commands[i] != NULL; i++) {
        // close previous commands file discriptors
        // base case
        if (commands[i][0] == NULL || is_delimiter(commands[i][0]) == 1 || cmd_index >= total_cmds ) {
            continue;
        } else if (strcmp(commands[i][0], "for") == 0) {
            last_val = run_for(commands, i, last_val);
            while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) {
                i++;
            }
        } else if (strcmp(commands[i][0], "if") == 0) {
            last_val = run_if(commands, i, last_val);
            while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) i++; // skip the to the end of the if block
            // check if there is an else statement
            if (commands[i + 1] != NULL && strcmp(commands[i + 1][0], "else") == 0) { // if there is an else statement
                i++;
                while (commands[i] != NULL && strcmp(commands[i][0], "}") != 0) i++; // skip the to the end of the else block
            }
        } else if (commands[i + 1] != NULL && strcmp(commands[i+1][0], "|") == 0) { // if the next delimiter is a pipe
            // pipe, so we fork()
            if (fork() == 0) {
                // child
                last_val = run_command(commands, commands[i], last_val, cmd_fd[cmd_index]);
                exit(last_val);
            }
            // parent
            wait(NULL);
            // close file descriptors now that the child is done
            if (cmd_fd[cmd_index][0] != STDIN_FILENO) close(cmd_fd[cmd_index][0]);
            if (cmd_fd[cmd_index][1] != STDOUT_FILENO) close(cmd_fd[cmd_index][1]);
            if (cmd_fd[cmd_index][2] != STDERR_FILENO) close(cmd_fd[cmd_index][2]);

            cmd_index++;
        } else {
            // no pipe, so we run the command
            last_val = run_command(commands, commands[i], last_val, cmd_fd[cmd_index]);
            cmd_index++;
            // if the next delimiter is a redirection, we skip the next delimiter, and the file name
            if (commands[i+1] != NULL && is_redirection_delimiter(commands[i+1][0]) == 1) {
                i += 2;
            } else if(commands[i+1] != NULL) {
                // skip next delimiter otherwise
                cmd_index--;
                i++;
            }
        }
        if (last_val == -1) break; // if the last command was interrupted by SIGINT
    }

    // clean up potential pipes
    for (int i = 0; cmd_fd[i] != NULL; i++) {
        for (int j = 0; j < 3; j++) {
            if (cmd_fd[i][j] != STDIN_FILENO && cmd_fd[i][j] != STDOUT_FILENO && cmd_fd[i][j] != STDERR_FILENO) {
                close(cmd_fd[i][j]);
            }
        }
        free(cmd_fd[i]);
    }
    free(cmd_fd);

    dup2(dup_stdin, STDIN_FILENO);
    dup2(dup_stdout, STDOUT_FILENO);
    dup2(dup_stderr, STDERR_FILENO);

    return last_val;
}
