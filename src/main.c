#include<stdio.h>
#include <string.h>
#include<prompt.h>
#include <tokenisation.h>
#include<stdlib.h>


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
  char* input = malloc(1000*sizeof(char)); // TODO: the max size should be changed later  
  while (1){
    if ( print_prompt(1) == -1){
    // error handling
      perror("error printing prompt in main.c");
      return -1;
    };
    if(input == NULL){
      perror("error allocating space in main.c");
      return -1;
    }
    fgets(input, 1000* sizeof(char), stdin);
    // tokenise the input
    char** tokens = tokenise(input);
    print_tokens(tokens);

    if (strcmp(tokens[0], "exit") == 0){
      // break out of the loop
      return 0;
    }
  }
}

