#include<stdio.h>
#include <string.h>
#include<prompt.h>
#include <tokenisation.h>
#include<stdlib.h>


int print_tokens(char** tokens, char* input){
  // on suppose que tokens est une liste valid de strings
  for(int i = 0 ; i < len_tokens(input); i++){
    printf("token %d : %s    ",i, tokens[i]);
  } 
  printf("\n");
}

int main(){
  int result;
  char* input = malloc(prompt_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later  
  while (1){
    result = print_prompt(1); // storing the result for potential debugging later on
    // error handling
    if(input == NULL){
      perror("error allocating space in main.c");
      return -1;
    }
    fgets(input, prompt_MAX_SIZE * sizeof(char), stdin);
    // tokenise the input
    char** tokens = tokenise(input);
    print_tokens(tokens, input);
    if (strcmp(tokens[0], "exit") == 0){
      // break out of the loop
      return 0;
    }
  }
}

