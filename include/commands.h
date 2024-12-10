#ifndef COMMANDS_H 
#define COMMANDS_H

extern int is_internal_command(char* command);
extern int run_commands(char*** commands, int last_val);
extern int run_command(char*** commands, char** command, int last_val, int input_fd, int output_fd, int error_fd);
extern void check_stdin_stdout();

#endif
