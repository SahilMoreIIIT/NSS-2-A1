/*
 * aclshell.c
 *
 * An interactive ACLShell that integrates ACL‐based commands:
 *   - getacl <file>                     
 *   - setacl <permissions> <user> <file>  
 *   - fget <file>                       
 *   - fput <file> <text>                
 *   - my_ls <directory>                 
 *   - my_cd <directory>                 
 *   - create_dir <directory>            
 *
 * Any other commands are passed to bash for normal processing.
 *
 * In addition, a detailed help function is provided which explains
 * the usage of each command and includes example invocations.
 *
 * Folder Owner Mappings (for ACL purposes):
 *   - India   : modi
 *   - China   : xi
 *   - America : trump
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
 * print_help: Displays detailed usage information and examples for the ACLShell.
 */
void print_help(void) {
    printf("\nACLShell User Manual\n");
    printf("---------------------\n");
    printf("This shell provides ACL-based functionality for file and directory operations.\n\n");
    printf("Available commands:\n");
    printf("  getacl <file>\n");
    printf("    Retrieves and displays the ACL for the specified file.\n");
    printf("    Example: getacl /ACLTesting/China/Alien.txt\n\n");

    printf("  setacl <permissions> <user> <file>\n");
    printf("    Sets the ACL for the specified file.\n");
    printf("    Permissions should be given as a string (e.g., rwx or a numeric code).\n");
    printf("    Example: setacl rwx modi /ACLTesting/India/China.txt\n\n");

    printf("  fget <file>\n");
    printf("    Reads and displays the contents of the specified file if ACL rules allow.\n");
    printf("    Example: fget /ACLTesting/America/Elon.txt\n\n");

    printf("  fput <file> <text>\n");
    printf("    Writes (or appends) the given text to the specified file, subject to ACL checks.\n");
    printf("    Example: fput /ACLTesting/America/War.txt \"New content added.\"\n\n");

    printf("  my_ls <directory>\n");
    printf("    Lists the contents of the specified directory if ACL rules permit access.\n");
    printf("    Example: my_ls /ACLTesting/China\n\n");

    printf("  my_cd <directory>\n");
    printf("    Changes directory to the specified directory after verifying ACL permissions.\n");
    printf("    Example: my_cd /ACLTesting/India\n\n");

    printf("  create_dir <directory>\n");
    printf("    Creates a new directory with ACL enforcement in the current context.\n");
    printf("    Example: create_dir /ACLTesting/India/NewDirectory\n\n");

    printf("Other commands entered in the shell are passed directly to bash for execution.\n\n");
    printf("Folder Owner Mappings (for ACL purposes):\n");
    printf("  India   : modi\n");
    printf("  China   : xi\n");
    printf("  America : trump\n\n");
    printf("Type 'exit' to quit the shell or 'help' to display this manual again.\n\n");
}

int main(void) {
    char cmd_line[MAX_CMD_LENGTH];
    char *tokens[MAX_TOKENS] = {0};  // Initialize tokens array

    while (1) {
        printf("ACLShell> ");
        fflush(stdout);

        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break;  // Exit on EOF (Ctrl-D)
        }

        // Remove trailing newline.
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Skip empty input.
        if (strlen(cmd_line) == 0)
            continue;

        // Tokenize the command line (splitting by whitespace).
        int token_count = 0;
        char *token = strtok(cmd_line, " ");
        while (token != NULL && token_count < MAX_TOKENS - 1) {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }
        tokens[token_count] = NULL;

        // Handle built-in commands.
        if (token_count == 0)
            continue;
        if (strcmp(tokens[0], "exit") == 0) {
            break;
        }
        if (strcmp(tokens[0], "help") == 0) {
            print_help();
            continue;
        }

        // List of custom ACL-based commands.
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
                // These binaries must be in a directory that is in the PATH.
                execvp(tokens[0], tokens);
                perror("execvp error");
                exit(EXIT_FAILURE);
            } else {
                // Fallback: execute command via bash.
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
            // Parent process: wait for child to finish.
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
