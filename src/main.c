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
#include <cd.h>

int main(){
  int last_val = 0; // value of the last command executed
  char* prompt = NULL;

    // Register the cleanup function
    if (atexit(cleanup) != 0) {
        perror("atexit failed");
        return 1;
    }

  //char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later
  //if(input == NULL){
  //  perror("error allocating space in main.c");
  //  return 1;
  //}
  while (1){
    prompt = getPrompt(last_val); 
    if(prompt == NULL){
      perror("error getting prompt in main.c");
      return 1;
    }
    rl_outstream = stderr;
    char* input = readline(prompt);
    if (input == NULL){
      free(prompt);
      return command_exit(NULL,last_val);
    }
    add_history(input);
    // tokenise the input
    char*** tokens = tokenise_cmds(input);

    //print_tokenised_cmds(tokens); // TODO: remove debug for tokens
    if (tokens != NULL && *tokens != NULL && **tokens != NULL){
      last_val = run_commands(tokens, last_val);
    }
    free(input);
    free(prompt);
    free_tokens(tokens);
  }
  return 0;
}

