#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/extattr.h>
#include <pwd.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;
    } acl[BUFFER_SIZE];
    int acl_count;
} ACL;

void ACL_init(ACL *acl) {
    acl->owner = -1;
    acl->acl_count = 0;
}

void ACL_set_owner(ACL *acl, uid_t owner) {
    acl->owner = owner;
}

uid_t ACL_get_owner(ACL *acl) {
    return acl->owner;
}

void ACL_add(ACL *acl, int user, int perms) {
    acl->acl[acl->acl_count].user = user;
    acl->acl[acl->acl_count].perms = perms;
    acl->acl_count++;
}

void ACL_remove(ACL *acl, int user) {
    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user) {
            for (int j = i; j < acl->acl_count - 1; j++) {
                acl->acl[j] = acl->acl[j + 1];
            }
            acl->acl_count--;
            return;
        }
    }
}

int ACL_check(ACL *acl, int user, int perms) {
    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user && (acl->acl[i].perms & perms) == perms) {
            return 1;
        }
    }
    return 0;
}

int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        perror("Error loading ACL");
        return 0;
    }
    acl->owner = getuid();
    return 1;
}

int ACL_save(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    snprintf(acl_str, sizeof(acl_str), "Owner: %d, ACL count: %d", acl->owner, acl->acl_count);

    if (extattr_set_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, strlen(acl_str)) == -1) {
        perror("Error saving ACL");
        return 0;
    }
    return 1;
}

int main() {
    ACL acl;
    ACL_init(&acl);  // Initialize the ACL

    ACL_set_owner(&acl, getuid());
    ACL_add(&acl, 1001, 7);

    const char *file = "/ACLTesting/India/China.txt";
    if (ACL_save(&acl, file)) {
        printf("ACL saved successfully to %s\n", file);
    } else {
        printf("Failed to save ACL.\n");
    }

    ACL acl_loaded;
    ACL_init(&acl_loaded);
    if (ACL_load(&acl_loaded, file)) {
        printf("ACL loaded successfully from %s\n", file);
    } else {
        printf("Failed to load ACL.\n");
    }

    if (ACL_check(&acl_loaded, 1003, 7)) {
        printf("User has permission.\n");
    } else {
        printf("User does not have permission.\n");
    }

    return 0;
}
