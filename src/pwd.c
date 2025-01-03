#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// include our own files
#include <pwd.h>


/**
 * @brief Function that prints the current working directory.
 * @param command the command to run
 * @return 0 if successful, 1 otherwise
 */
int command_pwd(char** command) {
    // check for extra arguments
    if(command[1] != NULL){
        // potentially deal with flags
        int len_flag = strlen(command[1]);
        char* output_string = malloc((25 + len_flag) * sizeof(char));
        if (output_string == NULL) {
            perror("error allocating space in main.c");
            return 1;
        }
        snprintf(output_string, 25+len_flag, "pwd: %s: invalid argument\n", command[1]);
        write(STDERR_FILENO, output_string, strlen(output_string));
        free(output_string);
        return 1;
    }
    // Initialize the working directory string
    char* dir_string = NULL;

    char* current_working_directory = getcwd(NULL, 0);
    if (current_working_directory == NULL) {
        perror("can't find current working directory in main.c");
        goto error;
    } else {
        size_t dir_len = strlen(current_working_directory) + 2;
        // allocate memory for the prompt, the +2 is for the '\n' and '\0'
        dir_string = malloc(dir_len);
        if (dir_string == NULL) { // Error handling
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
