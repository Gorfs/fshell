#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// personal files includes
#include <pwd.h>


// prints the current working directory to the file_descriptor,
// returns 1 if an error occurs, 0 otherwise
int command_pwd(char** command){
  // check for extra arguments
  if(command[1] != NULL){
    // potentially deal with flags
    int len_flag = strlen(command[1]);
    char* output_string = malloc((25 + len_flag) * sizeof(char));
    snprintf(output_string, 25+len_flag, "pwd: %s: invalid argument\n", command[1]);
    write(STDERR_FILENO, output_string, strlen(output_string));
    return 1;
  }
  // Initialize the working directory string
  char* dir_string = NULL;

  // getcwd() dynamically allocates memory for the path
  char* current_working_directory = getcwd(NULL, 0);
  if (current_working_directory == NULL){
    perror("can't find current working directory in main.c");
    goto error;
  }else{
    size_t dir_len = strlen(current_working_directory) + 2;
    // allocate memory for the prompt, the +2 is for the '\n' and '\0'
    dir_string = malloc(dir_len);
    if (dir_string == NULL) // error handling
    {
      perror("error allocating space in main.c");
      goto error;
    }
    snprintf(dir_string, dir_len, "%s\n", current_working_directory);
    if (write(STDOUT_FILENO, dir_string, strlen(dir_string)) == 1){
      perror("error writing prompt in main.c");
      goto error;
    }
  }
  // function executed succesfully, we free the dynamically allocated memory
  free(dir_string);
  free(current_working_directory);
  return 0;

error:
  free(dir_string);
  free(current_working_directory);
  return 1;
}
