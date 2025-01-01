#include <stdlib.h>
#include <tokenisation.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <commands.h>
#include <unistd.h>
#include <string.h>
#include <exit.h>
#include <pwd.h>
#include <ftype.h>
#include <cd.h> // temporary to use printf for debugging
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>


// list of internal commands
char* internal_commands[] = {"exit", "pwd", "cd", "ftype", NULL};
// list of ignored delimiters when it comes to command execution
char* redirection_delimiters[] = {">", ">>", "<", "<<","|>",">|", "|>>", "2>", "2>>","2>|", NULL};
/*
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
 * @brief repalce all occurences of old with new in string
 * @param str the string that we copy to make the new string
 * @param old the string that we replace with new
 * @param new_str replaced old in the str
 * @return char* copy of str with all occurences changed, NULL if error
 */
char* str_replace(const char* str, const char* old, const char* new_str) {
    // Check for null pointers
    if (str == NULL || old == NULL || new_str == NULL) {
        return NULL;
    }

    // Calculate the lengths of the strings
    size_t str_len = strlen(str);
    size_t old_len = strlen(old);
    size_t new_len = strlen(new_str);

    // If the old string is empty, return a copy of the original string
    if (old_len == 0) {
        return strdup(str);
    }

    // Count the number of times the old string occurs in the original string
    int count = 0;
    const char* tmp = str;
    while ((tmp = strstr(tmp, old)) != NULL) {
        count++;
        tmp += old_len;
    }

    // Calculate the length of the new string
    size_t new_str_len = str_len + count * (new_len - old_len) + 1;
    char* result = (char*)malloc(new_str_len);
    if (result == NULL) {
        perror("malloc");
        return NULL;
    }

    // Perform the replacement
    char* dst = result;
    const char* src = str;
    while (count > 0) {
        // Find the next occurrence of the old string
        tmp = strstr(src, old);
        if (tmp == NULL) {
            break;
        }

        // Copy the part of the string before the old string
        size_t len = tmp - src;
        strncpy(dst, src, len);
        dst += len;

        // Copy the new string
        strcpy(dst, new_str);
        dst += new_len;

        // Move the source pointer past the old string
        src = tmp + old_len;
        count--;
    }

    // Copy the remaining part of the original string
    strcpy(dst, src);

    return result;
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
 * @brief takes a file path, creates all directories in the path.
 * @param path : the path of the file
 * @return 0 if successful, -1 if an error occured
 */
int create_directory_file(char* path){
  char* path_copy = strdup(path);
  if(path_copy == NULL){
    perror("strdup");
    return -1;
  }
  char* token = strtok(path_copy, "/");
  char* token2 = strtok(NULL, "/");
  char* current_path = malloc(sizeof(char) * strlen(path) + 1);
  strcpy(current_path, token);
  while(token2 != NULL){
    // check if directory exists
    if(access(current_path, F_OK) == -1){
      // doesn't exist
      // create directory
      if(mkdir(current_path, 0777) == -1){
        perror("mkdir");
        return -1;
      }
    }
    // the directory now exists, we can move on to the next one
    current_path = strcat(current_path, "/");
    current_path = strcat(current_path, token2);
    token = token2;
    token2 = strtok(NULL, "/");
  }
  //int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  free(path_copy);
  free(current_path);
  return 0;
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
  int* fd = malloc(3 * sizeof(int));
  if(fd == NULL){
    perror("malloc");
    return NULL;
  }
  fd[0] = STDIN_FILENO;
  fd[1] = STDOUT_FILENO;
  fd[2] = STDERR_FILENO;
  // handle the possible redirection
  if(strcmp(delimiter, ">") == 0){
    // check if the file already exists
    if(access(file_name, F_OK) != -1){
      write(STDERR_FILENO, "pipeline_run:␣File␣exists\n", 30);
      fd[1] = -1;
    }else{
      if(create_directory_file(file_name) == -1){
        return NULL;
      }
      fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
  }else if (strcmp(delimiter, ">>") == 0){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if (strcmp(delimiter, "<") == 0){
    fd[0] = open(file_name, O_RDONLY);
  }else if (strcmp(delimiter, "2>") == 0){
    if(access(file_name, F_OK) != -1){
      write(STDERR_FILENO, "pipeline_run:␣File␣exists", 26);
      fd[2] = -1;
    }else{
      if(create_directory_file(file_name) == -1){
        return NULL;
      }
      fd[2] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
  }else if(strcmp(delimiter, "2>|") == 0){
    if(create_directory_file(file_name) == -1){
      return NULL;
    }
    fd[2] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if (strcmp(delimiter, "2>>") == 0){
    fd[2] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if(strcmp(delimiter, "<&") == 0){
    fd[0] = open(file_name, O_RDONLY);
  }else if(strcmp(delimiter, "|>") == 0){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if(strcmp(delimiter, "|>>") == 0){
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
  }else if(strcmp(delimiter, ">|") == 0){
    if(create_directory_file(file_name) == -1){
      return NULL;
    }
    fd[1] = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }else if(strcmp(delimiter, "<>") == 0){
    // pretty sure this one is not an actual redirection
    fd[0] = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
  } 
  if(fd[0] == -1 || fd[1] == -1 || fd[2] == -1){
    perror("open");
    goto error;
  }
  return fd;

 error:
  free(fd);
  return NULL;
}

/**
 * @brief initializes an array of file descriptors to stdin and stdout
 * @param fdArray : the array of filedescriptors, null terminated
 * @return 0 if  successful, -1 otherwise
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
  return -1;
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
    }    // if the command is a delimiter, we skip it after setting previous delim for the next iteration
    if(is_delimiter(commands[i][0])){
      previous_delimiter = commands[i][0];
      continue;
    }
    // if the previous delimiter is a redirection, we have skipped an input redirection, this is an edge case
    if(previous_delimiter != NULL && is_redirection_delimiter(previous_delimiter) == 1){
      cmd_index--;
      int* fd = handle_redirection(previous_delimiter, commands[i][0]);
      if(fd == NULL){ // error handeling
        return -1;
      }
      if(fdArray[cmd_index][2] == STDERR_FILENO){
        fdArray[cmd_index][2] = fd[2];
      }
      if(fdArray[cmd_index][0] == STDIN_FILENO){
        fdArray[cmd_index][0] = fd[0];
      }
      if(fdArray[cmd_index][1] == STDOUT_FILENO){
        fdArray[cmd_index][1] = fd[1];
      }
      free(fd);// free the pointer to the fd
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
      if(fdArray[cmd_index][2] == STDERR_FILENO){
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

int test_extension(char* full_path, char* extension){
  char* dot = strrchr(full_path, '.');
  if (dot == NULL || strcmp(dot + 1, extension) != 0) {
    return 0;
  }
  else{
    return 1;
  }
}

/**
 * @brief list all the files in a directory
 * @param path : the path of the directory
 * @return a list of strings containing the path of the files in the directory
 */
int count_files_recursive(char* path, int hidden_files, int recursive, char* extension, char type) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((entry->d_name[0] != '.' || (hidden_files == 1 && entry->d_name[0] == '.')) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            int path_len = strlen(path);
            int name_len = strlen(entry->d_name);
            int full_path_len = path_len + 1 + name_len + 1;

            char *full_path = malloc(full_path_len);
            if (full_path == NULL) {
                perror("malloc");
                closedir(dir);
                return count;
            }

            snprintf(full_path, full_path_len, "%s/%s", path, entry->d_name);

            struct stat st;
            if (lstat(full_path, &st) == 0) {
                int include = 1;
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
                if (extension != NULL){
                  is_extension = test_extension(full_path,extension);
                }

                if (include || (recursive && S_ISDIR(st.st_mode))) {
                    if (S_ISDIR(st.st_mode) && recursive) {
                        count += count_files_recursive(full_path, hidden_files, recursive, extension, type);
                    }
                    if ((extension == NULL && include) || (extension != NULL && is_extension == 1 && include)){
                      count++;
                    }
                }
            }
            free(full_path);
        }
    }
    closedir(dir);
    return count;
}

char** list_path_files(char* path, int hidden_files, int recursive, char* extension, char type) {
    int size = count_files_recursive(path, hidden_files, recursive, extension, type);
    char** files = malloc(sizeof(char*) * (size + 1));
    if (files == NULL) {
        perror("malloc");
        return NULL;
    }

    DIR *dir;
    struct dirent *entry;
    int index = 0;

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        free(files);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((entry->d_name[0] != '.' || (hidden_files == 1 && entry->d_name[0] == '.')) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            int path_len = strlen(path);
            int name_len = strlen(entry->d_name);
            int full_path_len = path_len + 1 + name_len + 1;

            char *full_path = malloc(full_path_len);
            if (full_path == NULL) {
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
                int include = 1;
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
                      if (extension == NULL || (extension != NULL && is_extension == 1)){
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
                        if (extension != NULL){
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
/**
 * @brief run a for loop
 *  @param commands : the list of commands
 *  @param i : the index of the for loop
 *  @param last_val : the value of the last command
 *  @return the value of the last command
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
    int max_loops = -1;

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
          max_loops = atoi(commands[i][j+1]);
          j++;  // Skip the next element as it's the number
        }
      }
    }

    char** list_of_path_files = list_path_files(rep_path_name, hidden_files, recursive, extension, type);
    if (list_of_path_files == NULL){
        perror("error in list_path_files");
        return 1;
    }

    int status = last_val;
    int max_status = 0;

    for (int j = 0; list_of_path_files[j] != NULL && (max_loops == -1 || j < max_loops); j++){
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
        if (status > max_status){
          max_status = status;
        }
        free_tokens(tokens);
    }

    for (int j = 0; list_of_path_files[j] != NULL; j++){
        free(list_of_path_files[j]);
    }
    free(list_of_path_files);
    free(final_var_name);
    return max_status;
}

int run_if(char*** commands, int i, int last_val) {
    // Checks if the "if" statement is followed by a condition
    int cmd_first_token_size = 0; // length of the first token
    for (int j = 0; commands[i][j] != NULL; j++) {
        cmd_first_token_size++;
    }
    if (cmd_first_token_size < 2) {
        perror("syntax error");
        return 2;
    }

    // Checks if the "if" statement is followed by a block
    if (strcmp(commands[i+1][0], "{") != 0){
        perror("if statement must be followed by a block");
        return 2;
    }

    // Checks if the "if" statement is closed with a "}"
    if (strcmp(commands[i+3][0], "}") != 0){
        perror("if statement must be closed with a }");
        return 2;
    }

    if (commands[i+4] != NULL && strcmp(commands[i+4][0], "else") == 0) { // make sure the syntax for "else" is correct
        if (commands[i+4][1] != NULL) {
            perror("syntax error");
            return 2;
        }

        // Checks if the "else" statement is followed by a block
        if (strcmp(commands[i+5][0], "{") != 0){
            perror("else statement must be followed by a block");
            return 2;
        }

        // Checks if the "else" statement is closed with a "}"
        if (strcmp(commands[i+7][0], "}") != 0){
            perror("else statement must be closed with a }");
            return 2;
        }
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


/**
 * @brief run a single command
 * @param commands : the list of commands (only required for command_exit())
 * @param command : the command to run
 * @param last_val : the value of the last command
 * @param input_fd : the input file descriptor
 * @param output_fd : the output file descriptor
 * @return the value of the last command
 */
int run_command(char*** commands, char** command, int last_val, int input_fd, int output_fd, int error_fd) {
  // check if the output file descriptor is valid
  if(output_fd == -1){
    return 1;
  }
  if(error_fd == -1){
    return 1;
  }
  if(input_fd == -1){
    return 1;
  }
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
  if (is_internal_command(command_name) == 1) {
    // Run the internal command
    if (strcmp(command_name, "exit") == 0) {
      status = command_exit(commands, command, status);
    } else if (strcmp(command_name, "pwd") == 0) {
      status = command_pwd(command);
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
  int dup_stdin = dup(STDIN_FILENO);
  int dup_stdout = dup(STDOUT_FILENO);
  int dup_stderr = dup(STDERR_FILENO);
  // setup descriptor array
  int total_cmds = length_of_total_input(commands);
  int** command_fileDescriptors = malloc(sizeof(int*) * (total_cmds + 1));// +1 for the null terminator
  int cmd_index = 0; 
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
    // base case
    if (commands[i][0] == NULL || is_delimiter(commands[i][0]) == 1 || cmd_index >= total_cmds ){
      continue;
    }else if (strcmp(commands[i][0], "for") == 0){
      last_val = run_for(commands, i, last_val);
      while(commands[i] != NULL && strcmp(commands[i][0], "}") != 0){
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
    } else if(commands[i + 1] != NULL && strcmp(commands[i+1][0], "|") == 0){ // if the next delimiter is a pipe
      // pipe, so we fork()
      if (fork() == 0){
        // child
        last_val = run_command(commands, commands[i], last_val, command_fileDescriptors[i][0], command_fileDescriptors[i][1], command_fileDescriptors[i][2]);
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
        if(command_fileDescriptors[i][2] != STDERR_FILENO){
          close(command_fileDescriptors[i][2]);
        }
        cmd_index++;
      }
    }else{
      // no pipe, so we run the command
      last_val = run_command(commands, commands[i], last_val, command_fileDescriptors[cmd_index][0], command_fileDescriptors[cmd_index][1], command_fileDescriptors[cmd_index][2]);
      cmd_index++;
      // if the next delimiter is a redirection, we skip the next delimiter, and the file name
      if(commands[i+1] != NULL && is_redirection_delimiter(commands[i+1][0]) == 1){
        i += 2;
      }else if(commands[i+1] != NULL){
        // skip next delimiter otherwise
        cmd_index--;
        i++; 
      }
    }
  }

  // clean up potential pipes
  
  for (int i = 0; command_fileDescriptors[i] != NULL; i++) {
    for (int j = 0; j < 3; j++) {
      if (command_fileDescriptors[i][j] != STDIN_FILENO && command_fileDescriptors[i][j] != STDOUT_FILENO && command_fileDescriptors[i][j] != STDERR_FILENO) {
        close(command_fileDescriptors[i][j]);
      }
    }
    free(command_fileDescriptors[i]);
  }
  free(command_fileDescriptors);

  dup2(dup_stdin, STDIN_FILENO);
  dup2(dup_stdout, STDOUT_FILENO);
  dup2(dup_stderr, STDERR_FILENO);
  
  return last_val;
}
