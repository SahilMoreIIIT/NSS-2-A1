#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/extattr.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>

#define BUFFER_SIZE 1024

// ---------------------------------------------------------------------
// ACL structure and functions (shared with fget.c/fput.c)
// ---------------------------------------------------------------------
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;  // Permission bits: 4 = read, 2 = write, 1 = execute
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

// For simplicity, this dummy ACL_load "deserializes" an ACL from the file's
// extended attributes. In a full implementation, you would parse a stored string.
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        // No ACL present.
        return 0;
    }
    // Dummy: Assume the ACL is valid and grants full permission (7) to current user.
    acl->owner = getuid();
    acl->acl_count = 1;
    acl->acl[0].user = getuid();
    acl->acl[0].perms = 7;
    return 1;
}

// ---------------------------------------------------------------------
// my_ls: List directory contents if the user has read permission.
// ---------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }
    
    const char *dir_path = argv[1];
    
    // Change effective UID to that of the directory owner.
    // For demonstration, assume current user is the owner.
    if (seteuid(getuid()) < 0) {
        perror("Error changing effective UID");
        return 1;
    }
    
    ACL acl;
    ACL_init(&acl);
    int acl_loaded = ACL_load(&acl, dir_path);
    int has_permission = 0;
    
    if (acl_loaded) {
        // Check if current user has read (bit 4) permission.
        if (ACL_check(&acl, getuid(), 4)) {
            has_permission = 1;
        }
    }
    
    // Fallback: if ACL isn't present or check fails, use DAC (access system call).
    if (!has_permission) {
        if (access(dir_path, R_OK) == 0) {
            has_permission = 1;
        }
    }
    
    if (!has_permission) {
        fprintf(stderr, "Permission denied: Cannot list directory %s\n", dir_path);
        return 1;
    }
    
    // Open the directory.
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return 1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        printf("%s\n", entry->d_name);
    }
    
    closedir(dir);
    
    // Restore effective UID.
    if (seteuid(getuid()) < 0) {
        perror("Error restoring effective UID");
        return 1;
    }
    
    return 0;
}
