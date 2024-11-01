#ifndef PROMPT_H
#define PROMPT_H

extern const char* prompt_suffix;
int print_prompt(int file_descriptor);
int prompt_MAX_SIZE = 1000; // TODO convert this to the number that is 1000 * sizeof(char)

#endif