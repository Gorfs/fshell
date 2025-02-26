# fshell

## Project Description

This project is a custom shell implementation for the SY5 course 2024-2025. It supports various built-in commands, pipelines, conditional statements (if), and loops (for).

## Authors

*   Beales Archie,  \@beales
*   Christophe JIN,  \@jinc
*   Cheze Remi,  \@cheze

## Architecture

The shell's architecture consists of the following main components:

*   **Prompt:** Generates the shell prompt, including the current directory, the status of the last command, and a signal indicator. ANSI color codes are used to improve readability (Blue = current directory, green = success, red = failure).
*   **Tokenization:** Divides a string of commands into tokens (array of arrays of strings). It handles tokenizing pipelines as well. A two-pointer search algorithm is used to identify delimiters and tokens, which also allows for the detection of if and for statements and nested curly braces.
*   **Commands:** Commands are treated as tokens and are managed by the shell if they are internal. Otherwise, external commands are executed with fork() and execvp(). Redirections (>, <, >>, etc.) are handled by modifying file descriptors (STDIN, STDOUT, STDERR) before command execution. Pipelines (|) are implemented by creating pipes and redirecting inputs and outputs between commands.
*   **Pipelines:** Manages the execution of multiple commands connected by pipelines (|). Each command is executed in a fork, and the outputs/inputs are redirected via pipes.
*   **If:** Executes conditional structures. It verifies the syntax of the structure and sends an error message if it is malformed.
*   **For:** Executes loops. It verifies the syntax of the loop and sends an error message if it is malformed. It supports options such as -A, -r, -e, -t, and -p. Parallelization is supported by forking multiple processes (according to the number given with -p) that execute their own command.

## Built-in Commands

*   **cd:** Changes the current directory. Supports changing to the last visited directory, the home directory, or a valid path.
*   **exit:** Exits the shell. If no argument is provided, it exits with the return value of the last command. If an argument is provided, it is converted to an integer and used as the exit code.
*   **pwd:** Prints the current directory.
*   **ftype:** Verifies and displays the type of the specified file.

## Building and Running

To build the project, use the following command:

```bash
make
```

To run the shell, execute the following command:

```bash
./fshell
```

## Testing

To run the tests, use the following command:

```bash
./test.sh

