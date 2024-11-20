#ifndef COMMANDS_H 
#define COMMANDS_H

extern int is_internal_command(char* command);
extern int run_command(char** command, int last_val);
extern int run_commands(char*** commands, int last_val, char* input);

#endif
