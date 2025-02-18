#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/extattr.h>
#include <errno.h>
#include <pwd.h>

#define BUFFER_SIZE 1024

// ---------------------------------------------------------------------
// ACL structure and functions (shared across fget, fput, my_ls)
// ---------------------------------------------------------------------
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;  // Permission bits: 4 = read, 2 = write, 1 = execute. Full permission = 7.
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

// Dummy ACL_load: attempts to read the ACL from the file's extended attributes.
// If found, populates a simple ACL structure (for demo purposes).
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        // No ACL found.
        return 0;
    }
    // For demonstration, assume the ACL is valid and grants full permission to current user.
    acl->owner = getuid();
    acl->acl_count = 1;
    acl->acl[0].user = getuid();
    acl->acl[0].perms = 7;
    return 1;
}

// Dummy ACL_save: saves a simple ACL string to the file's extended attributes.
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
// Helper function: print_ls
// Executes "ls -lha <path>" and prints its output.
// ---------------------------------------------------------------------
int print_ls(const char *path) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "ls -lha %s", path);
    
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        fprintf(stderr, "Error: Could not execute command: %s\n", command);
        return 0;
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        printf("%s", buffer);
    }
    
    pclose(pipe);
    return 1;
}

// ---------------------------------------------------------------------
// Helper function: validate_directory
// Checks if the provided path exists and is a directory.
// ---------------------------------------------------------------------
int validate_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("stat");
        return 0;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory\n", path);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------
// Helper function: get_dir_owner
// Retrieves the directory owner using stat().
// ---------------------------------------------------------------------
uid_t get_dir_owner(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("stat");
        return (uid_t)-1;
    }
    return st.st_uid;
}

// ---------------------------------------------------------------------
// my_cd: Change directory operation with ACL-based permission checking.
// ---------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }
    
    const char *path = argv[1];
    
    // Validate the directory.
    if (!validate_directory(path))
        return 1;
    
    // Get the ACL for the target directory.
    ACL acl;
    ACL_init(&acl);
    
    if (!ACL_load(&acl, path)) {
        // No ACL exists. Verify that the current user is the owner of the directory.
        uid_t dir_owner = get_dir_owner(path);
        if (dir_owner != getuid()) {
            fprintf(stderr, "Error: No ACL found and current user is not the owner of %s\n", path);
            return 1;
        }
        // Create a new ACL granting full permission (7) to the current user.
        ACL_set_owner(&acl, getuid());
        ACL_add(&acl, getuid(), 7);
        if (!ACL_save(&acl, path)) {
            fprintf(stderr, "Error: Failed to save new ACL for %s\n", path);
            return 1;
        }
    } else {
        // ACL exists; verify that the current user has execute (search) permission.
        // We map "execute" to permission bit 1.
        if (!ACL_check(&acl, getuid(), 1)) {
            fprintf(stderr, "Permission denied: You do not have execute access to %s\n", path);
            return 1;
        }
        
        // Change effective UID to that of the directory owner.
        if (seteuid(acl.owner) < 0) {
            fprintf(stderr, "Error: Could not change effective UID: %s\n", strerror(errno));
            return 1;
        }
    }
    
    printf("CD successful. Showing the contents of the directory:\n");
    printf("---------------------------------------------------------\n");
    
    if (!print_ls(path)) {
        fprintf(stderr, "Error: Failed to list directory contents.\n");
        return 1;
    }
    
    // Restore effective UID to real user.
    if (seteuid(getuid()) < 0) {
        fprintf(stderr, "Error: Could not restore effective UID: %s\n", strerror(errno));
        return 1;
    }
    
    return 0;
}
