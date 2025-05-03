#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

// Global variables for history
char *history[MAX_HISTORY];
int history_count = 0;
pid_t foreground_pid = 0;

// Signal handler for CTRL+C
void sigint_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    }
}

// Add command to history
void add_to_history(char *command) {
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(command);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

// Display history
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

// Parse command line into arguments
int parse_command(char *input, char **args) {
    int i = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

// Execute a single command
int execute_command(char **args, int input_fd, int output_fd) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) { // Child process
        // Handle input redirection
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        // Handle output redirection
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Execute command
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }
    
    foreground_pid = pid;
    int status;
    waitpid(pid, &status, 0);
    foreground_pid = 0;
    
    return WEXITSTATUS(status);
}

// Handle redirection
int handle_redirection(char **args, int *input_fd, int *output_fd) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            *input_fd = open(args[i + 1], O_RDONLY);
            if (*input_fd == -1) {
                perror("open");
                return -1;
            }
            args[i] = NULL;
            for (int j = i + 1; args[j] != NULL; j++) {
                args[j] = args[j + 1];
            }
        }
        else if (strcmp(args[i], ">") == 0) {
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd == -1) {
                perror("open");
                return -1;
            }
            args[i] = NULL;
            for (int j = i + 1; args[j] != NULL; j++) {
                args[j] = args[j + 1];
            }
        }
        else if (strcmp(args[i], ">>") == 0) {
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (*output_fd == -1) {
                perror("open");
                return -1;
            }
            args[i] = NULL;
            for (int j = i + 1; args[j] != NULL; j++) {
                args[j] = args[j + 1];
            }
        }
        i++;
    }
    return 0;
}


       

// Handle piping
void execute_pipeline(char *commands[][MAX_ARGS], int n_commands) {
    int fd[2];
    int input_fd = STDIN_FILENO;
    
    for (int i = 0; i < n_commands; i++) {
        pipe(fd);
        
        pid_t pid = fork();
        if (pid == 0) {
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (i < n_commands - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }
            close(fd[0]);
            close(fd[1]);
            
            execvp(commands[i][0], commands[i]);
            perror("execvp");
            exit(1);
        }
        
        close(fd[1]);
        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }
        input_fd = fd[0];
        
        foreground_pid = pid;
        wait(NULL);
        foreground_pid = 0;
    }
}

// ... (other functions remain the same)

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    
    // ... (signal handling and other code unchanged)
    
    while (1) {
        printf("sh> ");
        fflush(stdout);
        
        if (!fgets(input, MAX_INPUT, stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        add_to_history(input);
        
        if (strcmp(input, "history") == 0) {
            show_history();
            continue;
        }
        
        char *commands[MAX_ARGS];
        int n_commands = 0;
        char *token = strtok(input, ";");
        while (token != NULL && n_commands < MAX_ARGS - 1) {
            commands[n_commands++] = token;
            token = strtok(NULL, ";");
        }
        
        for (int i = 0; i < n_commands; i++) {
            char *and_commands[MAX_ARGS];
            int n_and_commands = 0;
            char *and_token = strtok(commands[i], "&&");
            while (and_token != NULL && n_and_commands < MAX_ARGS - 1) {
                and_commands[n_and_commands++] = and_token;
                and_token = strtok(NULL, "&&");
            }
            
            int last_status = 0;
            for (int j = 0; j < n_and_commands && last_status == 0; j++) {
                char *pipe_commands[MAX_ARGS];
                int n_pipe_commands = 0;
                char *pipe_token = strtok(and_commands[j], "|");
                while (pipe_token != NULL && n_pipe_commands < MAX_ARGS - 1) {
                    pipe_commands[n_pipe_commands++] = pipe_token;
                    pipe_token = strtok(NULL, "|");
                }
                
                if (n_pipe_commands > 1) {
                    char *pipe_args[MAX_ARGS][MAX_ARGS];
                    for (int k = 0; k < n_pipe_commands; k++) {
                        parse_command(pipe_commands[k], pipe_args[k]);
                    }
                    execute_pipeline(pipe_args, n_pipe_commands);
                } else {
                    int input_fd = STDIN_FILENO;
                    int output_fd = STDOUT_FILENO;
                    
                    parse_command(and_commands[j], args);
                    
                    if (handle_redirection(args, &input_fd, &output_fd) == -1) {
                        continue;
                    }
                    
                    if (strcmp(args[0], "exit") == 0) {
                        exit(0);
                    }
                    if (strcmp(args[0], "cd") == 0) {
                        if (args[1] == NULL) {
                            chdir(getenv("HOME"));
                        } else {
                            chdir(args[1]);
                        }
                        continue;
                    }
                    
                    last_status = execute_command(args, input_fd, output_fd);
                    
                    if (input_fd != STDIN_FILENO) {
                        close(input_fd);
                    }
                    if (output_fd != STDOUT_FILENO) {
                        close(output_fd);
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    
    return 0;
}
