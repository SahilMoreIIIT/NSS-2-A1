#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// Function to safely execute the command
void execute_command(char *cmd, char *argv[]) {
    execvp(cmd, argv);
    perror("execvp failed");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get real and effective user IDs
    uid_t real_uid = getuid();
    uid_t effective_uid = geteuid();

    // Verify that the target command exists
    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("stat failed");
        exit(EXIT_FAILURE);
    }

    // Check ownership of the target file
    if (sb.st_uid == 0) {
        // If the file is owned by root, retain root privileges (effective UID stays as root)
        if (seteuid(0) == -1) {
            perror("Failed to set effective UID to root");
            exit(EXIT_FAILURE);
        }
    } else {
        // If the file is NOT owned by root, drop privileges to the calling user
        if (seteuid(real_uid) == -1) {
            perror("Failed to drop privileges to real user");
            exit(EXIT_FAILURE);
        }
    }

    // Execute the command
    execute_command(argv[1], &argv[1]);

    return 0;
}
