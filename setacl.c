#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#define BUFFER_SIZE 1024

// Function to check if a user exists
int user_exists(const char *username) {
    struct passwd *pw = getpwnam(username);
    return pw != NULL;
}

// Function to set ACL for the file by writing to extended attributes
int set_acl(const char *file, const char *acl_str) {
    if (setxattr(file, "user.acl", acl_str, strlen(acl_str), 0) == -1) {
        perror("Error: Could not set ACL for file");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <perms> <user> <file>\n", argv[0]);
        return 1;
    }

    const char *perms = argv[1];
    const char *user = argv[2];
    const char *file = argv[3];

    // Validate user
    if (!user_exists(user)) {
        printf("Error: User '%s' does not exist\n", user);
        return 1;
    }

    // Create ACL string in the form of "user:user_name,perms"
    char acl_str[BUFFER_SIZE];
    snprintf(acl_str, BUFFER_SIZE, "user:%s,%s", user, perms);

    // Set ACL for the file
    if (!set_acl(file, acl_str)) {
        return 1;
    }

    printf("Successfully set ACL for user '%s' on file '%s' with permissions '%s'\n", user, file, perms);
    return 0;
}
