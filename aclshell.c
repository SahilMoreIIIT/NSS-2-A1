#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "acl.c"
#include "getacl.c"  // Include getacl.c
#include "setacl.c"  // Include setacl.c

#define MAX_CMD_LENGTH 256

void print_help() {
    printf("ACLShell Commands:\n");
    printf("  getacl <file>          - Get the ACL of a file\n");
    printf("  setacl <permissions> <user> <file> - Set ACL for a file\n");
    printf("  exit                   - Exit the shell\n");
}

int main() {
    char cmd[MAX_CMD_LENGTH];
    char user[MAX_CMD_LENGTH];
    char file[MAX_CMD_LENGTH];
    char perms[MAX_CMD_LENGTH];

    while (1) {
        printf("ACLShell> ");
        if (!fgets(cmd, MAX_CMD_LENGTH, stdin)) {
            perror("Error reading command");
            continue;
        }

        cmd[strcspn(cmd, "\n")] = '\0';  // Remove trailing newline

        if (strncmp(cmd, "getacl", 6) == 0) {
            // Command: getacl <file>
            if (sscanf(cmd, "getacl %s", file) == 1) {
                getacl(file); // Call getacl function to fetch ACL
            } else {
                printf("Usage: getacl <file>\n");
            }
        } 
        else if (strncmp(cmd, "setacl", 6) == 0) {
            // Command: setacl <permissions> <user> <file>
            if (sscanf(cmd, "setacl %s %s %s", perms, user, file) == 3) {
                setacl(perms, user, file); // Call setacl function to modify ACL
            } else {
                printf("Usage: setacl <permissions> <user> <file>\n");
            }
        } 
        else if (strcmp(cmd, "exit") == 0) {
            // Command: exit
            break;
        } 
        else {
            print_help();
        }
    }

    return 0;
}
