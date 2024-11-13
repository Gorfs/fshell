#ifndef TOKENISATION_H
#define TOKENISATION_H

extern char** tokenise(char* input, char delimiter); // returns a list of tokens from the input string followed by a NULL terminator
extern int len_tokens(char* input, char delimiter);

#endif
