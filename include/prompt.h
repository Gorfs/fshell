#ifndef PROMPT_H
#define PROMPT_H

extern const char* prompt_suffix;
extern char* getPrompt(int last_val, int is_sig);
#define PROMPT_MAX_SIZE 1000
#endif