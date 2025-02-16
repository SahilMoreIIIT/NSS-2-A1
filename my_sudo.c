#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// Assumptions:
// - The target command is provided as the first argument (with an absolute or relative path).
// - The program is installed as a setuid-root binary so that its effective UID is initially 0.
// - The caller has provided a valid command; proper error messages are printed if not.

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Retrieve the real and effective user IDs.
    uid_t real_uid = getuid();
    uid_t effective_uid = geteuid();
    
    // Verify that the target command exists and is executable.
    if (access(argv[1], X_OK) != 0) {
        perror("access");
        exit(EXIT_FAILURE);
    }
    
    // Obtain file status of the target command to check its ownership.
    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    
    // If the target command is not owned by root (i.e. non-root owned),
    // drop privileges by switching to the real (non-root) user.
    if (sb.st_uid != 0) {
        // Temporarily drop privileges using seteuid().
        if (seteuid(real_uid) == -1) {
            perror("seteuid");
            exit(EXIT_FAILURE);
        }
        // Permanently drop privileges using setuid().
        if (setuid(real_uid) == -1) {
            perror("setuid");
            exit(EXIT_FAILURE);
        }
    } else {
        // Optional: If the target is owned by root, ensure we remain with elevated privileges.
        if (seteuid(0) == -1) {
            perror("seteuid to root");
            exit(EXIT_FAILURE);
        }
    }
    
    // Execute the target command with any additional arguments.
    if (execvp(argv[1], &argv[1]) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    // This point is not reached unless execvp() fails.
    return 0;
}
