#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/extattr.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// Define ACL structure
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;
    } acl[BUFFER_SIZE];
    int acl_count;
} ACL;

// Function to initialize ACL
void ACL_init(ACL *acl) {
    acl->owner = -1;
    acl->acl_count = 0;
}

// Function to set ACL owner
void ACL_set_owner(ACL *acl, uid_t owner) {
    acl->owner = owner;
}

// Function to load ACL from file's extended attributes
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        perror("Error loading ACL");
        return 0;
    }
    acl->owner = getuid();
    return 1;
}

// Function to verify ACL for the current user and permissions
int ACL_check(ACL *acl, int user, const char *perms) {
    int required_perm = 0;
    if (strcmp(perms, "r") == 0) {
        required_perm = 4;  // Read permission (example)
    } else if (strcmp(perms, "w") == 0) {
        required_perm = 2;  // Write permission (example)
    } else if (strcmp(perms, "x") == 0) {
        required_perm = 1;  // Execute permission (example)
    } else {
        fprintf(stderr, "Invalid permission requested: %s\n", perms);
        return 0;
    }

    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user && (acl->acl[i].perms & required_perm) == required_perm) {
            return 1;
        }
    }
    return 0;
}

// Function to read from a file based on ACL validation
int fget(const char *file) {
    // Validate the file
    if (file == NULL || strlen(file) == 0) {
        fprintf(stderr, "File path cannot be empty\n");
        return 0;
    }

    // Load ACL
    ACL acl;
    ACL_init(&acl);
    if (!ACL_load(&acl, file)) {
        fprintf(stderr, "Failed to load ACL for file: %s\n", file);
        return 0;
    }

    // Verify access rights (Read permission required for the current user)
    if (!ACL_check(&acl, getuid(), "r")) {
        fprintf(stderr, "Permission denied: User does not have read access\n");
        return 0;
    }

    // Open and read the file
    FILE *in_file = fopen(file, "r");
    if (in_file == NULL) {
        perror("Error opening file");
        return 0;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), in_file)) {
        printf("%s", line);
    }

    fclose(in_file);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];

    // Change to owner of the file (to emulate changing user ID)
    if (seteuid(getuid()) < 0) {
        perror("Error: Could not change to owner of the file");
        return 1;
    }

    // Fetch and display file contents if permissions are granted
    if (!fget(file)) {
        fprintf(stderr, "Failed to fetch file contents\n");
        return 1;
    }

    // Change back to original user ID
    if (seteuid(getuid()) < 0) {
        perror("Error: Could not change back to user");
        return 1;
    }

    return 0;
}
