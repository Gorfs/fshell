#ifndef COMMANDS_H 
#define COMMANDS_H

extern int is_internal_command(char* command);
extern int run_command(char*** commands, char** command, int last_val, int intput_fd, int output_fd);
extern int run_commands(char*** commands, int last_val);

#endif
