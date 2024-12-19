#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <exit.h>

void free_tokens(char*** tokens){
  if (tokens == NULL){
    return;
  }
  for(int i = 0; tokens[i] != NULL; i++){
    if (tokens[i] != NULL) {
      for (int j = 0; tokens[i][j] != NULL; j++){
        if (tokens[i][j] != NULL) {
          free(tokens[i][j]);
        }
      }
      free(tokens[i]);
    }
  }
  free(tokens);
}

int command_exit(char*** commands, char** command, int last_val){
  
  if(command == NULL || command[1] == NULL){
    if (commands != NULL){
      free_tokens(commands);
    }
    exit(last_val);
  }else{
    if(command[2] != NULL){
      write(STDERR_FILENO, "exit: too many arguments\n", 25);
      return 1;
    }
    int exit_code = atoi(command[1]);
    free_tokens(commands);
    exit(exit_code);
  }
}
