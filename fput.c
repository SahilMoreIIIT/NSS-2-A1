#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/extattr.h>
#include <pwd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// ---------------------------------------------------------------------
// ACL structure and functions (similar to fget.c)
// ---------------------------------------------------------------------
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms; // Assume: 4 = read, 2 = write, 1 = execute; e.g., 7 = full permission.
    } acl[BUFFER_SIZE];
    int acl_count;
} ACL;

void ACL_init(ACL *acl) {
    acl->owner = (uid_t)-1;
    acl->acl_count = 0;
}

void ACL_set_owner(ACL *acl, uid_t owner) {
    acl->owner = owner;
}

void ACL_add(ACL *acl, int user, int perms) {
    if (acl->acl_count < BUFFER_SIZE) {
        acl->acl[acl->acl_count].user = user;
        acl->acl[acl->acl_count].perms = perms;
        acl->acl_count++;
    }
}

int ACL_check(ACL *acl, int user, int required_perm) {
    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user && (acl->acl[i].perms & required_perm) == required_perm) {
            return 1;
        }
    }
    return 0;
}

// For simplicity, this function loads a dummy ACL.
// In a real scenario you would deserialize a stored ACL string.
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        // No ACL exists on file.
        return 0;
    }
    // Dummy deserialization: assume the ACL is valid and contains one entry
    // with the current user having full permission.
    acl->owner = getuid();
    acl->acl_count = 1;
    acl->acl[0].user = getuid();
    acl->acl[0].perms = 7;  // Full permission
    return 1;
}

// ---------------------------------------------------------------------
// fput: Write text to a file if the user has write permission
// ---------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> \"<text>\"\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];
    const char *text = argv[2];

    // Verify that the file exists.
    if (access(file, F_OK) != 0) {
        fprintf(stderr, "Error: File %s does not exist\n", file);
        return 1;
    }

    // Change effective UID to that of the file owner.
    // In this demo we assume the file owner is the current user.
    if (seteuid(getuid()) < 0) {
        perror("Error: Could not change to file owner");
        return 1;
    }

    ACL acl;
    ACL_init(&acl);

    // Try to load the ACL from the file.
    if (!ACL_load(&acl, file)) {
        // No ACL exists. Create a new ACL for the file.
        ACL_set_owner(&acl, getuid());
        // Grant full permission (7) to the current user.
        ACL_add(&acl, getuid(), 7);
        // (In a complete system you might want to save this ACL to the file.)
    } else {
        // ACL exists; check if the current user has write permission.
        // In our mapping, write permission is represented by bit 2.
        if (!ACL_check(&acl, getuid(), 2)) {
            fprintf(stderr, "Permission denied: You do not have write access to %s\n", file);
            return 1;
        }
    }

    // Open the file for writing (truncation mode).
    FILE *fp = fopen(file, "w");
    if (!fp) {
        perror("Error opening file for writing");
        return 1;
    }

    // Write the provided text to the file.
    if (fprintf(fp, "%s\n", text) < 0) {
        perror("Error writing to file");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    // Restore effective UID to the real user.
    if (seteuid(getuid()) < 0) {
        perror("Error: Could not restore user privileges");
        return 1;
    }

    printf("Successfully wrote to %s\n", file);
    return 0;
}
