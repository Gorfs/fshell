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

int command_exit(char*** commands, int i, int last_val){
    if(commands == NULL || *commands == NULL || (*commands)[1] == NULL){
        if (commands != NULL){
            free_tokens(commands);
        }
        exit(last_val);
    }
    else{
        int exit_code = atoi(commands[i][1]);
        free_tokens(commands);
        exit(exit_code);
    }
}