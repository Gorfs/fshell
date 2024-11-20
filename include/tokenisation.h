#ifndef TOKENISATION_H
#define TOKENISATION_H

#define MAX_COMMAND_SIZE 100
extern int len_tokens(char* input);
extern int len_cmds(char* input);
extern char** delimiters_in_order(char* input);
extern char*** tokenise_input(char* input); // returns a list of commands from the input string followed by a NULL terminator

#endif
