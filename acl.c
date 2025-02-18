#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <pwd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// Structure to hold ACL
typedef struct {
    uid_t owner;
    char acl[BUFFER_SIZE];  // Store ACL in a string (for simplicity)
} ACL;

// Function to load ACL from extended attributes
int load_acl(const char* file, ACL* acl) {
    ssize_t acl_size;
    acl_size = getxattr(file, "user.acl", acl->acl, sizeof(acl->acl));

    if (acl_size == -1) {
        perror("Error loading ACL");
        return -1;
    }

    // For simplicity, assume ACL is just a string in the extended attributes
    printf("ACL loaded for file: %s\n", file);
    return 0;
}

// Function to save ACL to extended attributes
int save_acl(const char* file, const ACL* acl) {
    if (setxattr(file, "user.acl", acl->acl, strlen(acl->acl), 0) == -1) {
        perror("Error saving ACL");
        return -1;
    }

    printf("ACL saved for file: %s\n", file);
    return 0;
}

// Function to check if the user has permission for a file
int check_permission(const ACL* acl, uid_t user, const char* perm) {
    // For simplicity, assume the ACL is in string format like "user:uid=rw-"
    if (strstr(acl->acl, "rw-") != NULL) {
        return 1;  // Allow if the ACL contains rw- permissions
    }
    return 0;  // Deny otherwise
}

// Function to get file owner
uid_t get_owner(const char* file) {
    struct stat stat_buf;
    if (stat(file, &stat_buf) == -1) {
        perror("Error getting file status");
        return -1;
    }
    return stat_buf.st_uid;
}

// Main driver for getacl
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    const char* file = argv[1];
    ACL acl;

    if (load_acl(file, &acl) == -1) {
        return 1;
    }

    // Here we simply print the ACL (you can customize it as needed)
    printf("ACL for file %s: %s\n", file, acl.acl);

    return 0;
}

// Main driver for setacl
int set_acl(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -[rwx] <user> <file>\n", argv[0]);
        return 1;
    }

    const char* perms = argv[1];
    const char* user = argv[2];
    const char* file = argv[3];

    // Create or load ACL
    ACL acl;
    if (load_acl(file, &acl) == -1) {
        return 1;
    }

    // Update the ACL string (simplified example)
    strcat(acl.acl, " user:");
    strcat(acl.acl, user);
    strcat(acl.acl, "=");
    strcat(acl.acl, perms);

    // Save the modified ACL back to the file
    if (save_acl(file, &acl) == -1) {
        return 1;
    }

    printf("ACL updated for file: %s\n", file);
    return 0;
}
