#ifndef PROMPT_H
#define PROMPT_H

extern const char* prompt_suffix;
extern int print_prompt(int file_descriptor, int last_val);
#define PROMPT_MAX_SIZE 1000
#endif