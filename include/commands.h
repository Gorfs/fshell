#ifndef COMMANDS_H 
#define COMMANDS_H

extern int is_internal_command(char* command);
extern int run_command(char*** commands, int i, int last_val);
extern int run_commands(char*** commands, int last_val);

#endif
