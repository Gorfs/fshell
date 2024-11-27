#include <unistd.h> // used for getcwd()
#include <prompt.h> // even though technically not needed, it's good practice to include the header file of the source file
#include <stdlib.h> // used for malloc()
#include <stdio.h> // used for debugging mostly
#include <string.h> // used for strlen()


const char* prompt_suffix = "$ ";

// colors for the prompt
const char* cwd_color = "\033[0;34m";
const char* reset = "\033[0m";


// prints the prompt the file_descriptor, returns 1 if an error occurs, 0 otherwise./
char *getPrompt(int last_val){
    char* prompt = NULL;

    // getcwd() dynamically allocates memory for the path, On doit donc la free apres que on l'as utiliser.
    char* current_working_directory = getcwd(NULL, 0);
    if (current_working_directory == NULL){
        perror("can't find current working directory in prompt.c");
        goto error;
    }else {
        // Shorten the prompt if the path is too long
        size_t max_size_cwd = 25 - 3; // -3 for the "..." that will be added
        size_t last_val_len = 0;
        if (last_val != 0){
            size_t temp = last_val;
            while (temp > 10){
                temp /= 10;
                last_val_len++;
            }
        }
        max_size_cwd -= last_val_len;
        if (strlen(current_working_directory) > (max_size_cwd)){
            char* temp = malloc(max_size_cwd + 3 + 1); // +3 for the "..." and +1 for the \0
            if (temp == NULL){
                perror("error allocating space in prompt.c");
                goto error;
            }
            strcpy(temp, "...");
            strcat(temp, current_working_directory + (strlen(current_working_directory) - max_size_cwd));
            free(current_working_directory);
            current_working_directory = temp;
        }
        // the extra characters are the \0 , " ", and the recent execution status and +16 for the prompt color codes
        int prompt_len = strlen(current_working_directory) + strlen(prompt_suffix) + 6 + 16;
        // allocation de la memoire pour le string du prompt
        prompt = malloc(prompt_len);
        // error handling
        if (prompt == NULL){
            perror("error allocating space in prompt.c");
            goto error;
        }
        char* val_color = NULL;
        if (last_val == 0){
            val_color = "\033[0;32m"; // green for success
        }else{
            val_color = "\033[0;91m"; // red for error
        }
        // Construct the prompt using snprintf
        snprintf(prompt, prompt_len, "%s[%d]%s%s%s%s",val_color ,last_val, cwd_color, current_working_directory, reset, prompt_suffix);
    }

    // function executed succesfully
    free(current_working_directory);
    return prompt;

    // freeing the dynamically allocated memory
    error:
        free(prompt);
        free(current_working_directory);
        return NULL;
    
}