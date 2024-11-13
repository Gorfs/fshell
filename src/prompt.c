#include <unistd.h> // used for getcwd()
#include <prompt.h> // even though technically not needed, it's good practice to include the header file of the source file
#include <stdlib.h> // used for malloc()
#include <stdio.h> // used for debugging mostly
#include <string.h> // used for strlen()


const char* prompt_suffix = "$ ";

// prints the prompt the file_descriptor, returns -1 if an error occurs, 0 otherwise./
int print_prompt(int file_descriptor){
    char* prompt = NULL;

    // getcwd() dynamically allocates memory for the path, On doit donc la free apres que on l'as utiliser.
    char* current_working_directory = getcwd(NULL, 0);
    if (current_working_directory == NULL){
        perror("can't find current working directory in prompt.c");
        goto error;
    }else {
        // Shorten the prompt if the path is too long
        size_t max_size_cwd = 25 - 3; // -3 for the "..." that will be added
        if (strlen(current_working_directory) > max_size_cwd){
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

        int prompt_len = strlen(current_working_directory) + strlen(prompt_suffix) + 6; // the extra characteurs are the \0 , " ", and the recent execution status
        // allocation de la memoire pour le string du prompt
        prompt = malloc(prompt_len);
        // error handling
        if (prompt == NULL){
            perror("error allocating space in prompt.c");
            goto error;
        }
        char* status = getenv("?");
        if (status == NULL){
            status = "0";
        }
         // Construct the prompt using snprintf
        snprintf(prompt, prompt_len, "[%s]%s%s",status, current_working_directory, prompt_suffix);
        if (write(file_descriptor, prompt, strlen(prompt)) == -1){
            perror("error writting prompt in prompt.c");
            goto error;
        }
    }

    // function executed succesfully
    free(prompt);
    free(current_working_directory);
    return 0; 

    // freeing the dynamically allocated memory
    error:
        free(prompt);
        free(current_working_directory);
        return -1;
    
}