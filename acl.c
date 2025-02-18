#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

// Structure to represent an ACL entry
typedef struct {
    uid_t owner;
    int acl[BUFFER_SIZE]; // User ACLs
} ACL;

// Helper function to get username from uid
char *name_from_uid(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return pw->pw_name;
    }
    return NULL;
}

// Function to serialize the ACL structure into a string
int serialize_acl(ACL *acl, char *buffer) {
    int pos = 0;
    pos += snprintf(buffer + pos, BUFFER_SIZE - pos, "Owner: %s\n", name_from_uid(acl->owner));
    pos += snprintf(buffer + pos, BUFFER_SIZE - pos, "ACL:\n");

    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (acl->acl[i] != 0) {
            pos += snprintf(buffer + pos, BUFFER_SIZE - pos, "  User %d: %d\n", i, acl->acl[i]);
        }
    }
    return pos;
}

// Function to load ACL from extended attributes of a file
int load_acl(const char *file, ACL *acl) {
    ssize_t len;
    char buffer[BUFFER_SIZE];
    len = getxattr(file, "user.acl", buffer, BUFFER_SIZE);
    if (len == -1) {
        perror("Failed to load ACL");
        return -1;
    }

    sscanf(buffer, "Owner: %d", &(acl->owner)); // Assuming ACL format is simple
    // Parse ACL entries from the buffer (simple format)
    return 0;
}

// Function to save ACL to extended attributes of a file
int save_acl(const char *file, ACL *acl) {
    char buffer[BUFFER_SIZE];
    int len = serialize_acl(acl, buffer);
    if (setxattr(file, "user.acl", buffer, len, 0) == -1) {
        perror("Failed to set ACL");
        return -1;
    }
    return 0;
}

// Function to check if a user has specific permissions
int check_acl(ACL *acl, int user, int perm) {
    if (acl->acl[user] & perm) {
        return 1; // Permission granted
    }
    return 0; // Permission denied
}

// Function to add user with permissions to the ACL
void add_acl_user(ACL *acl, int user, int perms) {
    acl->acl[user] = perms;
}

// Function to remove a user from the ACL
void remove_acl_user(ACL *acl, int user) {
    acl->acl[user] = 0;
}

// Main program for setting ACL
int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file> <user> <perms>\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];
    const char *user_name = argv[2];
    int perms = atoi(argv[3]);

    // Simulate user to UID mapping
    struct passwd *pw = getpwnam(user_name);
    if (!pw) {
        fprintf(stderr, "User not found: %s\n", user_name);
        return 1;
    }
    uid_t user_uid = pw->pw_uid;

    ACL acl;
    if (load_acl(file, &acl) == -1) {
        fprintf(stderr, "No ACL found for file: %s\n", file);
        // If no ACL exists, create a new one
        acl.owner = getuid();
        memset(acl.acl, 0, sizeof(acl.acl)); // Initialize with no users
    }

    // Add user with specified permission
    add_acl_user(&acl, user_uid, perms);

    // Save ACL back to file
    if (save_acl(file, &acl) == -1) {
        fprintf(stderr, "Failed to save ACL to file\n");
        return 1;
    }

    printf("ACL updated successfully for file: %s\n", file);
    return 0;
}

// Function to implement the getacl command
int get_acl(const char *file) {
    ACL acl;
    if (load_acl(file, &acl) == -1) {
        fprintf(stderr, "No ACL found for file: %s\n", file);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    int len = serialize_acl(&acl, buffer);
    printf("%s", buffer);
    return 0;
}

// Function to implement the checkacl command
int check_acl_permission(const char *file, int user, int perm) {
    ACL acl;
    if (load_acl(file, &acl) == -1) {
        fprintf(stderr, "No ACL found for file: %s\n", file);
        return 1;
    }

    if (check_acl(&acl, user, perm)) {
        printf("User %d has permission %d\n", user, perm);
    } else {
        printf("User %d does not have permission %d\n", user, perm);
    }
    return 0;
}
