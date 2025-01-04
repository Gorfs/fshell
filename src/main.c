#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// include our own files
#include <cd.h>
#include <commands.h>
#include <exit.h>
#include <prompt.h>
#include <tokenisation.h>


// value of the last command executed, volatile because it's modified by the signal handler
volatile sig_atomic_t last_val = 0;
volatile sig_atomic_t is_sig = 0;
int signals[2] = {SIGINT, SIGTERM}; // signals to handle


void signal_handler(int sig) {
    if (sig == SIGINT) {
        last_val = 255; // set the last value to 255
        is_sig = 1;
    } else if (sig == SIGTERM) {
        return; // Ignore SIGTERM
    }
}


void init_signal_handler() {
    struct sigaction action;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}


/**
 * @brief Main function of the shell that continuously reads input from the user
 * @return the value of the last command used before exiting
 */
int main() {
    char *prompt = NULL; // initialise the prompt

    if (atexit(cleanup) != 0) { // Register the cleanup function
        perror("atexit failed");
        return 1;
    }

    // initialise the signal handler
    init_signal_handler();

    while (1) {
        prompt = getPrompt(last_val, is_sig);
        is_sig = 0; // reset the signal boolean
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
