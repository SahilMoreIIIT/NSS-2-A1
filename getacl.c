#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#define BUFFER_SIZE 1024

// Function to load ACL from the extended attributes of the file
int load_acl(const char *file, char *acl_str) {
    ssize_t acl_attr = getxattr(file, "user.acl", acl_str, BUFFER_SIZE);
    if (acl_attr == -1) {
        perror("Error: No ACL found for the file");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];
    char acl_str[BUFFER_SIZE];

    if (!load_acl(file, acl_str)) {
        return 1;
    }

    // Print the retrieved ACL
    printf("ACL for file '%s':\n%s\n", file, acl_str);
    return 0;
}
