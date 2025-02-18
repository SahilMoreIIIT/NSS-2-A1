#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/extattr.h>
#include <errno.h>
#include <limits.h>

#define BUFFER_SIZE 1024

// ---------------------------------------------------------------------
// ACL structure and functions (shared with other ACL utilities)
// ---------------------------------------------------------------------
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;  // Permission bits: 4 = read, 2 = write, 1 = execute; full = 7.
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

// Dummy ACL_load: attempts to load an ACL from the file's extended attributes.
// For simplicity, if an ACL exists, we assume it grants full permission (7) to the current user.
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t ret = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (ret == -1) {
        // No ACL found.
        return 0;
    }
    acl->owner = getuid();
    acl->acl_count = 1;
    acl->acl[0].user = getuid();
    acl->acl[0].perms = 7;
    return 1;
}

// ACL_save: saves a simple ACL string to the file's extended attributes.
int ACL_save(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    snprintf(acl_str, sizeof(acl_str), "Owner: %d, ACL count: %d", acl->owner, acl->acl_count);
    if (extattr_set_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, strlen(acl_str)) == -1) {
        perror("Error saving ACL");
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------
// validate_owner: Checks if the current user is the owner of the given path.
// ---------------------------------------------------------------------
int validate_owner(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("stat");
        return 0;
    }
    return (st.st_uid == getuid());
}

// ---------------------------------------------------------------------
// Main: Create a directory with ACL enforcement.
// ---------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <dir_name>\n", argv[0]);
        return 1;
    }

    const char *new_dir = argv[1];

    // Get current working directory (the parent directory where new_dir will be created)
    char *cwd = getcwd(NULL, 0);
    if (!cwd) {
        perror("getcwd");
        return 1;
    }

    // Load ACL for the current directory.
    ACL current_acl;
    ACL_init(&current_acl);
    if (!ACL_load(&current_acl, cwd)) {
        // No ACL exists. Ensure current user is the owner.
        if (!validate_owner(cwd)) {
            fprintf(stderr, "Error: No ACL found and current user is not the owner of %s\n", cwd);
            free(cwd);
            return 1;
        }
        // Create a new ACL for the current directory granting full permission to current user.
        ACL_set_owner(&current_acl, getuid());
        ACL_add(&current_acl, getuid(), 7);
        if (!ACL_save(&current_acl, cwd)) {
            fprintf(stderr, "Error: Failed to save ACL for current directory %s\n", cwd);
            free(cwd);
            return 1;
        }
    } else {
        // ACL exists; check if current user has write permission (mapped as bit 2).
        if (!ACL_check(&current_acl, getuid(), 2)) {
            fprintf(stderr, "Permission denied: You do not have write access in %s\n", cwd);
            free(cwd);
            return 1;
        }
        // Change effective UID to that of the directory owner.
        if (seteuid(current_acl.owner) < 0) {
            fprintf(stderr, "Error: Could not change effective UID: %s\n", strerror(errno));
            free(cwd);
            return 1;
        }
    }

    // Create the new directory with mode 0755.
    if (mkdir(new_dir, 0755) == -1) {
        perror("mkdir");
        free(cwd);
        return 1;
    }

    // Set ACL for the new directory: grant full permission (7) to current user.
    ACL new_acl;
    ACL_init(&new_acl);
    ACL_set_owner(&new_acl, getuid());
    ACL_add(&new_acl, getuid(), 7);
    if (!ACL_save(&new_acl, new_dir)) {
        fprintf(stderr, "Error: Could not save ACL for new directory %s\n", new_dir);
        free(cwd);
        return 1;
    }

    // Restore effective UID to the real user.
    if (seteuid(getuid()) < 0) {
        fprintf(stderr, "Error: Could not restore effective UID: %s\n", strerror(errno));
        free(cwd);
        return 1;
    }

    printf("Directory '%s' created successfully with ACL enforcement.\n", new_dir);
    free(cwd);
    return 0;
}
