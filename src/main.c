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
#include <fcntl.h>

int main()
{
  int last_val = 0; // value of the last command executed
  char *prompt = NULL;

  // Register the cleanup function
  if (atexit(cleanup) != 0)
  {
    perror("atexit failed");
    return 1;
  }

  // char* input = malloc(PROMPT_MAX_SIZE*sizeof(char)); // TODO: the max size should be changed later
  // if(input == NULL){
  //   perror("error allocating space in main.c");
  //   return 1;
  // }
  while (1)
  {
    prompt = getPrompt(last_val);
    if (prompt == NULL)
    {
      perror("error getting prompt in main.c");
      return 1;
    }
    rl_outstream = stderr;

    char *input = readline(prompt);
    if (input == NULL)
    {
      free(prompt);
      command_exit(NULL, 0, last_val);
    }
    add_history(input);
    // tokenise the input
    char ***tokens = tokenise_cmds(input);

    if (input != NULL)
    {
      free(input);
    }
    if (prompt != NULL)
    {
      free(prompt);
    }
    //print_tokenised_cmds(tokens); // TODO: remove debug for tokens
    // seperate the commands in between the ; delimiters
    int i = 0;
    int previous_cut_index = -1;
    for (i = 0; tokens != NULL && tokens[i] != NULL && tokens[i][0] != NULL; i++)
    {
      if (*tokens[i][0] == ';')
      {
        tokens[i] = NULL;
        last_val = run_commands(tokens + previous_cut_index + 1, last_val);
        previous_cut_index = i;
        i++;
      }
    }
    // run the last command
    if (tokens != NULL)
    {
      last_val = run_commands(tokens + previous_cut_index + 1, last_val);
    }
    if (tokens != NULL)
    {
      free_tokens(tokens);
    }
  }
  return 0;
}