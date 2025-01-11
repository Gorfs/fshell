#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
// include our own files
#include <commands.h>
#include <pipeline.h>
#include <tokenisation.h>
#include <exit.h>


int run_one_pipe(char ***new_cmd, int last_val, int *fd) {
    int input_fd = fd[0];
    int output_fd = fd[1];

    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            return 1;
        }
        close(output_fd);
    }

    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            return 1;
        }
        close(input_fd);
    }

    last_val = run_commands(new_cmd, last_val); // run the command
    return last_val;
}


/**
 * Function that runs a simple commands with pipeline.
 * @param commands   array of array of strings representing the commands, supposed to be only simple commands
 * @param last_val   the value of the last command
 * @param i          pointer to the index of the current command
 * @param total_cmds total number of simple commands
 * @param cmd_fd     array of file descriptors for input, output and error for each command
 * @return return value of the last command
 */
int run_pipeline(char ***commands, int last_val, int i, int total_cmds, int **cmd_fd) {
    pid_t *pids = malloc(sizeof(pid_t) * total_cmds);
    if (pids == NULL) {
        perror("malloc");
        return 1;
    }
    int status = last_val;
    for (int j = 0; j < total_cmds; j++) {
        char *** new_cmd = tokenise_cmds(commands[i][0]); // tokenise the command
        if (new_cmd == NULL) {
            perror("error tokenising the command");
            free(pids);
            return 1;
        }
        pids[j] = fork();
        if (pids[j] == -1) {
            perror("fork");
            free(pids);
            return 1;
        }

        if (pids[j] == 0) { // child process number [j]
            last_val = run_one_pipe(new_cmd, last_val, cmd_fd[j]); // run the command
            free_tokens(new_cmd); // free the tokens
            exit(last_val); // exit the child process
        }
        free_tokens(new_cmd); // free the tokens
        // parent process update the indexes
        i += 2;
    }

    for (int j = 0; j < total_cmds; j++) {
        waitpid(pids[j], &status, 0);

        if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) == SIGINT) status = -1;
            else if (WTERMSIG(status) == SIGTERM) status = -2;
        } else {
            status = WEXITSTATUS(status);
        }

        // close file descriptors now that the child is done
        if (cmd_fd[j][0] != STDIN_FILENO) close(cmd_fd[j][0]);
        if (cmd_fd[j][1] != STDOUT_FILENO) close(cmd_fd[j][1]);
        if (cmd_fd[j][2] != STDERR_FILENO) close(cmd_fd[j][2]);
    }

    free(pids); // all the children are done

    return status;
}