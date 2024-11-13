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