#include <stdio.h>
#include <string.h>
#include <prompt.h>
#include <commands.h>
#include <pwd.h>
#include <tokenisation.h>
#include <stdlib.h>
#include <exit.h>

// TODO: DEBUGGING FUNCTION TO BE REMOVED LATER 
// on peut garder la fonction si on enleve le printf et on utiliser notre fonction IO
void print_tokens(char** tokens){
  // on suppose que tokens est une liste valid de strings
  for(int i = 0 ; tokens[i] != NULL; i++){
    printf("token %d : %s    ",i, tokens[i]);
  } 
  printf("\n");
}

int main(){
  int output = 2; // output where we write the prompt, 2 is the standard error output
  int last_val = 0; // value of the last command executed

  char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later
  while (1){
    if ( print_prompt(output) == -1){
    // error handling
      perror("error printing prompt in main.c");
      return -1;
    }
    if(input == NULL){
      perror("error allocating space in main.c");
      return -1;
    }
    fgets(input, PROMPT_MAX_SIZE* sizeof(char), stdin);

    if (feof(stdin)){
      return command_exit(last_val);
    }
    // tokenise the input
    char** tokens = tokenise(input);
    print_tokens(tokens);
    run_command(tokens);
    if (strcmp(tokens[0], "exit") == 0){
      // break out of the loop
      if (tokens[1] != NULL) 
      {
	  int val = atoi(tokens[1]);
	  free(tokens);
	  return val;
      }
      free(tokens);
      return 0;
    }else{

    }
  }
}

