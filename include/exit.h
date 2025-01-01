#ifndef EXIT_H
#define EXIT_H

extern void free_tokens(char*** tokens);
extern int command_exit(char*** commands, char** command, int last_val);

#endif // EXIT_H