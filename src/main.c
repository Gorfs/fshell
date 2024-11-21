#include <stdio.h>
#include <string.h>
#include <prompt.h>
#include <commands.h>
#include <pwd.h>
#include <tokenisation.h>
#include <stdlib.h>
#include <exit.h>
#include <unistd.h>
#include <cd.h>

// TODO: DEBUGGING FUNCTION TO BE REMOVED LATER 
// on peut garder la fonction si on enleve le printf et on utiliser notre fonction IO
void print_tokens(char** tokens){
  // on suppose que tokens est une liste valid de strings
  for(int i = 0 ; tokens[i] != NULL; i++){
    printf("token %d : \"%s\"    ",i, tokens[i]);
  } 
  printf("\n");
}

int main(){
  int output = STDERR_FILENO; // output where we write the prompt, 2 is the standard error output
  int last_val = 0; // value of the last command executed

    // Register the cleanup function
    if (atexit(cleanup) != 0) {
        perror("atexit failed");
        return 1;
    }

  char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later
  while (1){
    if (print_prompt(output, last_val) == 1){
    // error handling
      perror("error printing prompt in main.c");
      return 1;
    }
    if(input == NULL){
      perror("error allocating space in main.c");
      return 1;
    }
    fgets(input, PROMPT_MAX_SIZE* sizeof(char), stdin);
    // tokenise the input
    char** tokens = tokenise(input);
    if (feof(stdin)){
      return command_exit(NULL,last_val);
    }
    //printf("input : %s\n", input);
    //print_tokens(tokens);
    last_val = run_command(tokens, last_val);
    free(tokens);
  }
  return 0;
}

