#include <stdio.h>
#include <string.h>
#include <prompt.h>
#include <commands.h>
#include <pwd.h>
#include <tokenisation.h>
#include <stdlib.h>
#include <exit.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h> 

// TODO: DEBUGGING FUNCTION TO BE REMOVED LATER 
// on peut garder la fonction si on enleve le printf et on utiliser notre fonction IO
void print_tokens(char** tokens){
  // on suppose que tokens est une liste valid de strings
  for(int i = 0 ; tokens[i] != NULL; i++){
    printf("token %d : \"%s\"    ",i, tokens[i]);
  } 
  printf("\n");
}
void print_cmds(char*** tokenized_cmds){
  for(int i = 0; tokenized_cmds[i] != NULL; i++){
    printf("command %d : ", i);
    print_tokens(tokenized_cmds[i]);
  }
}

int main(){
  int last_val = 0; // value of the last command executed
  char* prompt = NULL;

  char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later
  if(input == NULL){
    perror("error allocating space in main.c");
    return 1;
  }
  while (1){
    prompt = getPrompt(last_val); 
    if(prompt == NULL){
      perror("error getting prompt in main.c");
      free(input);
      return 1;
    }
    input = readline(prompt);
    // tokenise the input
    if (input == NULL){
      perror("error reading input in main.c");
      free(input);
      return 1;
    }else if(strlen(input) <= 1){ 
      // command is null or empty, just ignore it
      continue;
    }
    char*** tokens = tokenise_input(input);

    if (feof(stdin)){
      free(input);
      return command_exit(*tokens,last_val);
    }

    //printf("input : %s\n", input);
    print_cmds(tokens);
    last_val = run_commands(tokens, last_val, input);
    free(tokens);
  }
  return 0;
}

