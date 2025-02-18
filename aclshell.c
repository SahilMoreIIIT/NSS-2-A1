/*
 * aclshell.c
 *
 * An interactive ACLShell that integrates ACL-based commands:
 *   - getacl <file>
 *   - setacl <permissions> <user> <file>
 *   - fget <file>
 *   - fput <file> <text>
 *   - my_ls <directory>
 *   - my_cd <directory>
 *   - create_dir <directory>
 *
 * For all custom commands, the shell forks and executes the respective
 * binary (which must have been installed with the setuid bit enabled).
 * Any other commands are passed to bash.
 *
 * Author: [Your Name]
 * Date: [Current Date]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD_LENGTH 256
#define MAX_TOKENS 16

/* 
 * print_help: Displays usage information for the ACLShell.
 */
void print_help() {
    printf("ACLShell Commands:\n");
    printf("  getacl <file>                     - Get the ACL of a file\n");
    printf("  setacl <permissions> <user> <file>  - Set ACL for a file\n");
    printf("  fget <file>                       - Read a file with ACL check\n");
    printf("  fput <file> <text>                - Write (append) to a file with ACL check\n");
    printf("  my_ls <directory>                 - List directory contents with ACL check\n");
    printf("  my_cd <directory>                 - Change directory with ACL check\n");
    printf("  create_dir <directory>            - Create a directory with ACL enforcement\n");
    printf("  exit                              - Exit the shell\n");
    printf("  help                              - Show this help message\n");
}

int main() {
    char cmd_line[MAX_CMD_LENGTH];
    char *tokens[MAX_TOKENS];
    
    while (1) {
        printf("ACLShell> ");
        fflush(stdout);
        
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break;  // Exit on EOF (Ctrl-D)
        }
        
        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Skip empty input
        if (strlen(cmd_line) == 0)
            continue;
        
        // Tokenize the command line using whitespace as delimiter.
        int token_count = 0;
        char *token = strtok(cmd_line, " ");
        while (token != NULL && token_count < MAX_TOKENS - 1) {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }
        tokens[token_count] = NULL;
        
        // Check for built-in commands.
        if (token_count == 0)
            continue;
        
        if (strcmp(tokens[0], "exit") == 0) {
            break;
        }
        
        if (strcmp(tokens[0], "help") == 0) {
            print_help();
            continue;
        }
        
        /* List of custom ACL-based commands.
         * These commands must correspond to your separate binaries.
         */
        const char *custom_cmds[] = {
            "getacl", "setacl", "fget", "fput", "my_ls", "my_cd", "create_dir", NULL
        };
        int is_custom = 0;
        for (int i = 0; custom_cmds[i] != NULL; i++) {
            if (strcmp(tokens[0], custom_cmds[i]) == 0) {
                is_custom = 1;
                break;
            }
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            continue;
        } else if (pid == 0) {
            // In child process.
            if (is_custom) {
                // Execute the custom ACL command binary.
                // These binaries should be in the PATH.
                execvp(tokens[0], tokens);
                perror("execvp error");
                exit(EXIT_FAILURE);
            } else {
                // Fallback: execute the command via bash.
                // Reconstruct the command line.
                char command_str[MAX_CMD_LENGTH] = "";
                for (int i = 0; i < token_count; i++) {
                    strcat(command_str, tokens[i]);
                    if (i < token_count - 1)
                        strcat(command_str, " ");
                }
                char *bash_args[] = {"/bin/bash", "-c", command_str, NULL};
                execv("/bin/bash", bash_args);
                perror("execv error");
                exit(EXIT_FAILURE);
            }
        } else {
            // In parent process: wait for the child to finish.
            int status;
            waitpid(pid, &status, 0);
        }
    }
    
    return 0;
}
