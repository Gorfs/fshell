#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// include our own files
#include <cd.h>
#include <commands.h>
#include <exit.h>
#include <prompt.h>
#include <tokenisation.h>

/**
 * @brief Main function of the shell that continuously reads input from the user
 * @return the value of the last command used before exiting
 */
int main() {
    int last_val = 0; // value of the last command executed
    char *prompt = NULL;

    if (atexit(cleanup) != 0) { // Register the cleanup function
        perror("atexit failed");
        return 1;
    }

    while (1) {
        prompt = getPrompt(last_val);
        if (prompt == NULL) {
            perror("error getting prompt in main.c");
            return 1;
        }
        rl_outstream = stderr;

        char *input = readline(prompt);
        if (input == NULL) {
            free(prompt);
            command_exit(NULL, 0, last_val);
        }
        add_history(input);
        // tokenise the input
        char ***tokens = tokenise_cmds(input);

        if (input != NULL) free(input); // free the input if it's not null
        free(prompt); // free the prompt to reset it for the next iteration

        // seperate the commands in between the ; delimiters
        int i = 0;
        int previous_cut_index = -1;
        for (i = 0; tokens != NULL && tokens[i] != NULL && tokens[i][0] != NULL; i++) {
            if (*tokens[i][0] == ';') {
                tokens[i] = NULL;
                last_val = run_commands(tokens + previous_cut_index + 1, last_val);
                previous_cut_index = i;
                i++;
            }
        }
        // run the last command
        if (tokens != NULL) {
            last_val = run_commands(tokens + previous_cut_index + 1, last_val);
            free_tokens(tokens);
        }
    }
}
