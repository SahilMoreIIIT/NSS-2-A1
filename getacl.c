#include <sys/types.h>
#include <sys/extattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: getacl <file>\n");
        return 1;
    }

    const char *file = argv[1];
    char acl_str[BUFFER_SIZE];
    ssize_t acl_len;

    // Retrieve extended attribute (user.acl) from the file
    acl_len = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_len == -1) {
        perror("Error getting ACL");
        return 1;
    }

    // Null-terminate the retrieved ACL string
    acl_str[acl_len] = '\0';

    // Print the retrieved ACL
    printf("ACL for file '%s': %s\n", file, acl_str);
    return 0;
}
