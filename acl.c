#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/extattr.h>  // Updated header for extended attributes
#include <pwd.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

// Define structure for ACL
typedef struct ACL {
    uid_t owner;
    struct {
        int user;
        int perms;
    } acl[BUFFER_SIZE];
    int acl_count;
} ACL;

// Function to initialize ACL
void ACL_init(ACL *acl) {
    acl->owner = -1;
    acl->acl_count = 0;
}

// Function to set owner of ACL
void ACL_set_owner(ACL *acl, uid_t owner) {
    acl->owner = owner;
}

// Function to get owner of ACL
uid_t ACL_get_owner(ACL *acl) {
    return acl->owner;
}

// Function to add user-permission pair to ACL
void ACL_add(ACL *acl, int user, int perms) {
    acl->acl[acl->acl_count].user = user;
    acl->acl[acl->acl_count].perms = perms;
    acl->acl_count++;
}

// Function to remove user from ACL
void ACL_remove(ACL *acl, int user) {
    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user) {
            // Shift remaining ACL entries to remove the user
            for (int j = i; j < acl->acl_count - 1; j++) {
                acl->acl[j] = acl->acl[j + 1];
            }
            acl->acl_count--;
            return;
        }
    }
}

// Function to check permissions for a user in ACL
int ACL_check(ACL *acl, int user, int perms) {
    for (int i = 0; i < acl->acl_count; i++) {
        if (acl->acl[i].user == user && (acl->acl[i].perms & perms) == perms) {
            return 1;
        }
    }
    return 0;
}

// Function to load ACL from extended attributes of a file
int ACL_load(ACL *acl, const char *file) {
    char acl_str[BUFFER_SIZE];
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, sizeof(acl_str));
    if (acl_attr == -1) {
        perror("Error loading ACL");
        return 0;
    }
    // Deserialize ACL here
    // For simplicity, assuming we just read it as a string and add it to ACL structure
    // You could implement more complex deserialization logic as needed
    acl->owner = getuid(); // Example: setting owner to current user
    return 1;
}

// Function to save ACL to extended attributes of a file
int ACL_save(ACL *acl, const char *file) {
    // For simplicity, just converting ACL structure to string
    // Implement real serialization if required
    char acl_str[BUFFER_SIZE];
    snprintf(acl_str, sizeof(acl_str), "Owner: %d, ACL count: %d", acl->owner, acl->acl_count);
    
    if (extattr_set_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, strlen(acl_str)) == -1) {
        perror("Error saving ACL");
        return 0;
    }
    return 1;
}

