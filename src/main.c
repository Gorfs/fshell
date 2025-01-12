#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
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


void signal_handler() {
    last_val = 255; // set the last value to 255
    is_sig = 1;
}


void setup_signal_handlers() {
    struct sigaction action;

    // Configurer gestionnaire pour SIGINT
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; // Pas de comportement spécial
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("Erreur sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    }

    // Ignorer SIGTERM
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGTERM, &action, NULL) == -1) {
        perror("Erreur sigaction(SIGTERM)");
        exit(EXIT_FAILURE);
    }
}


void reset_signals() {
    struct sigaction action;
    action.sa_flags = 0;
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("Erreur sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &action, NULL) == -1) {
        perror("Erreur sigaction(SIGTERM)");
        exit(EXIT_FAILURE);
    }
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

    setup_signal_handlers(); // setup the signal handlers

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
                // set up the signal mask before running the commands
                // run the commands
                last_val = run_commands(tokens + previous_cut_index + 1, last_val);
                if (last_val == -1) {
                    last_val = 255;
                    is_sig = 1;
                    break;
                }
                previous_cut_index = i;
                i++;
            }
        }
        // run the last command
        if (tokens != NULL) {
            // run the commands
            last_val = run_commands(tokens + previous_cut_index + 1, last_val);
            if (last_val == -1 || last_val == -2) {
                last_val = 255;
                is_sig = 1;
            }
            // unblock the signals after running the commands
            free_tokens(tokens);
        }
    }
}
