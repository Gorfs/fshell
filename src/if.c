#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// include our own files
#include <commands.h>
#include <exit.h>
#include <if.h>
#include <tokenisation.h>


int run_if(char*** commands, int i, int last_val) {
    // Checks if the "if" statement is followed by a condition
    int cmd_first_token_size = 0; // length of the first token
    for (int j = 0; commands[i][j] != NULL; j++) {
        cmd_first_token_size++;
    }
    if (cmd_first_token_size < 2) {
        perror("syntax error");
        return 2;
    }

    // Checks if the "if" statement is followed by a block
    if (strcmp(commands[i+1][0], "{") != 0){
        perror("if statement must be followed by a block");
        return 2;
    }

    // Checks if the "if" statement is closed with a "}"
    if (strcmp(commands[i+3][0], "}") != 0){
        perror("if statement must be closed with a }");
        return 2;
    }

    if (commands[i+4] != NULL && strcmp(commands[i+4][0], "else") == 0) { // make sure the syntax for "else" is correct
        if (commands[i+4][1] != NULL) {
            perror("syntax error");
            return 2;
        }

        // Checks if the "else" statement is followed by a block
        if (strcmp(commands[i+5][0], "{") != 0){
            perror("else statement must be followed by a block");
            return 2;
        }

        // Checks if the "else" statement is closed with a "}"
        if (strcmp(commands[i+7][0], "}") != 0){
            perror("else statement must be closed with a }");
            return 2;
        }
    }

    // Get the status of the last command
    int status = last_val;

    // Init a new array to store the condition
    char ***if_condition = malloc(sizeof(char**) * 2);
    if (if_condition == NULL) {
        perror("malloc");
        return 1;
    }
    // Set the condition to the command after the "if" statement
    if_condition[0] = commands[i] + 1;
    if_condition[1] = NULL;

    // Run the condition
    status = run_commands(if_condition, status);

    if (status == 0) { // if the status is 0, then the condition is true
        // Init a new array to store the block of commands if the condition is true
        char ***if_then = tokenise_cmds(commands[i+2][0]); // Run the block of commands if the condition is true
        status = run_commands(if_then, status);
        free_tokens(if_then);

    } else if (commands[i+4] != NULL && strcmp(commands[i+4][0], "else") == 0) { // if the status is not 0, then the condition is false
        // Init a new array to store the block of commands if the condition is false
        char ***if_else = tokenise_cmds(commands[i+6][0]);
        // Run the block of commands after the "else" statement
        status = run_commands(if_else, status);
        free_tokens(if_else);
    } else {
        status = 0; // if there is no "else" statement, the status is 0
    }

    free(if_condition);
    return status;
}
