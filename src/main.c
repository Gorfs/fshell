#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// personal files includes
#include <prompt.h>
#include <tokenisation.h>
#include <pwd.h>

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
  char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later  
  while (1)
  {
    if ( print_prompt(1) == -1){
      // error handling
      perror("error printing prompt in main.c");
      return -1;
    }
    if(input == NULL){
      perror("error allocating space in main.c");
      return -1;
    }
    fgets(input, PROMPT_MAX_SIZE* sizeof(char), stdin);
    // tokenise the input
    char** tokens = tokenise(input);
    print_tokens(tokens);

    if (strcmp(tokens[0], "pwd") == 0) // Check if the first token is "pwd"
    {
      if (command_pwd(1) == -1)
      {
        perror("error executing pwd command in main.c");
        return -1;
      }
    }

    if (strcmp(tokens[0], "exit") == 0){
      // break out of the loop
      return 0;
    }
  }
}

