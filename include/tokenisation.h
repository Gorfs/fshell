#ifndef TOKENISATION_H
#define TOKENISATION_H

#define MAX_COMMAND_SIZE 100
extern int len_tokens(char* input);
extern int len_cmds(char* input);
extern char*** tokenise_cmds(char* input);
extern void print_tokenised_cmds(char*** tokenised_cmds);
extern int is_delimiter(char* potential_delimiter);

#endif
