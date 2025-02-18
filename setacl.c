#include <sys/types.h>
#include <sys/extattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: setacl <perms> <user> <file>\n");
        return 1;
    }

    const char *perms = argv[1];
    const char *user = argv[2];
    const char *file = argv[3];
    char acl_str[BUFFER_SIZE];

    // Prepare the ACL string to be set (e.g., user:username,rw-)
    snprintf(acl_str, sizeof(acl_str), "user:%s,%s", user, perms);

    // Set extended attribute (user.acl) to the file
    if (extattr_set_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, strlen(acl_str)) == -1) {
        perror("Error setting ACL");
        return 1;
    }

    printf("Successfully set ACL for file '%s'\n", file);
    return 0;
}
