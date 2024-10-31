#include <prompt.h>
// used for getcwd()
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


const char* prompt_suffix = "fshell> ";

void print_prompt(int file_descriptor){
    char* prompt;
    char* current_working_directory = getcwd(NULL, 0);
    if (current_working_directory == NULL){
        perror("getcwd");
        return;
    }else {
        int prompt_len = strlen(current_working_directory) + strlen(prompt_suffix) + 3; // the 3 extra characteurs are the \0 , " ", ">"
        // allocation de la memoire pour le string du prompt
        prompt = malloc(prompt_len);
        // error handling
        if (prompt == NULL){
            perror("malloc");
            return;
        }
         // Construct the prompt using snprintf
        snprintf(prompt, prompt_len, "%s> %s", current_working_directory, prompt_suffix);
        write(file_descriptor, prompt, strlen(prompt));
    }
}