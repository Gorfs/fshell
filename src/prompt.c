#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// include our own files
#include <prompt.h>


// prompt suffix
const char* prompt_suffix = "$ ";

// colors for the prompt
const char* cwd_color = "\001\033[34m\002";
const char* reset = "\001\033[00m\002";


/**
 * @brief Function that returns the prompt for the shell.
 * @param last_val the value of the last command
 * @param is_sig 1 if there was a signal between the last prompt and this one, 0 otherwise
 * @return returns 1 if an error occurs, 0 otherwise
 */
char *getPrompt(int last_val, int is_sig){
    char* prompt = NULL;
    char* current_working_directory = getcwd(NULL, 0);

    if (current_working_directory == NULL) {
        perror("can't find current working directory in prompt.c");
        goto error;
    } else {
        // Shorten the prompt if the path is too long
        size_t max_size_cwd = 25 - 3; // -3 for the "..." that will be added
        size_t last_val_len = 0;
        if (is_sig == 1) {
            // 3 characters long to write "SIG", not 3 cuz we don't add to the size if last_val is between 0 and 9
            last_val_len = 2;
        } else if (last_val != 0) {
            size_t temp = last_val;
            while (temp > 10) {
                temp /= 10;
                last_val_len++;
            }
        }
        max_size_cwd -= last_val_len;
        if (strlen(current_working_directory) > (max_size_cwd)) {
            char* temp = malloc(max_size_cwd + 3 + 1); // +3 for the "..." and +1 for the \0
            if (temp == NULL) {
                perror("error allocating space in prompt.c");
                goto error;
            }
            strcpy(temp, "...");
            strcat(temp, current_working_directory + (strlen(current_working_directory) - max_size_cwd));
            free(current_working_directory);
            current_working_directory = temp;
        }
        // the extra characters are the \0 , " ", and the recent execution status and +20 for the prompt color codes
        int prompt_len = strlen(current_working_directory) + strlen(prompt_suffix) + last_val_len + 5 + 20;
        // allocation de la memoire pour le string du prompt
        prompt = malloc(prompt_len);
        // error handling
        if (prompt == NULL){
            perror("error allocating space in prompt.c");
            goto error;
        }
        char* val_color = NULL;
        if (last_val == 0) {
            val_color = "\001\033[32m\002"; // green for success
        } else {
            val_color = "\001\033[91m\002"; // red for error
        }
        if (is_sig == 1) {
            snprintf(prompt, prompt_len, "%s[SIG]%s%s%s%s", val_color, cwd_color, current_working_directory, reset, prompt_suffix);
        } else {
            // Construct the prompt using snprintf
            snprintf(prompt, prompt_len, "%s[%d]%s%s%s%s",val_color ,last_val, cwd_color, current_working_directory, reset, prompt_suffix);
        }
    }

    // function executed successfully
    free(current_working_directory);
    return prompt;

    // freeing the dynamically allocated memory
    error:
        free(prompt);
        free(current_working_directory);
        return NULL;
}
