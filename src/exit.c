#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <exit.h>

void free_tokens(char*** tokens){
  if (tokens == NULL){
    return;
  }
  for(int i = 0; tokens[i] != NULL; i++){
    for (int j = 0; tokens[i][j] != NULL; j++){
      free(tokens[i][j]);
    }
    free(tokens[i]);
  }
  free(tokens);
}

int command_exit(char*** command, int last_val){
    if(command == NULL || *command == NULL || (*command)[1] == NULL){
        if (command != NULL){
            free_tokens(command);
        }
        exit(last_val);
    }
    else{
        int exit_code = atoi((*command)[1]);
        free_tokens(command);
        exit(exit_code);
    }
}