#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// personal files includes
#include <prompt.h>
#include <tokenisation.h>
#include <pwd.h>
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

    if (strcmp(tokens[0], "pwd") == 0) // Check if the first token is "pwd"
    {
      last_val = command_pwd(output);
      if (last_val == -1)
      {
        perror("error executing pwd command in main.c");
        return -1;
      }
    }

    if (strcmp(tokens[0], "exit") == 0){
      if (len_tokens(input) > 2){
        write(STDERR_FILENO, "error: too many arguments\n", 27);
      }
      else if (len_tokens(input) == 2){
        return command_exit(atoi(tokens[1]));
      }
      else{
        return command_exit(last_val);
      }
    }
  }
}

