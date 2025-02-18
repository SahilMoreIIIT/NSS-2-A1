#include <sys/types.h>
#include <sys/extattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

// Structure to hold ACL data (simplified version for this example)
typedef struct ACL {
    uid_t owner;
    // A simple structure to store ACL entries (user_id, permissions)
    // In real scenarios, this would be a more complex structure.
    struct {
        uid_t user;
        int perms;
    } entries[10];  // Limit to 10 ACL entries for this example
    int entry_count;
} ACL;

// Function to serialize ACL (convert to string)
char* serialize_acl(ACL* acl) {
    char* serialized = (char*)malloc(BUFFER_SIZE);
    memset(serialized, 0, BUFFER_SIZE);

    snprintf(serialized, BUFFER_SIZE, "Owner: %d\n", acl->owner);
    for (int i = 0; i < acl->entry_count; i++) {
        snprintf(serialized + strlen(serialized), BUFFER_SIZE - strlen(serialized), 
                 "User: %d, Perms: %d\n", acl->entries[i].user, acl->entries[i].perms);
    }

    return serialized;
}

// Function to deserialize ACL (convert from string)
int deserialize_acl(ACL* acl, const char* serialized_acl) {
    sscanf(serialized_acl, "Owner: %d", &(acl->owner));
    return 0;
}

// Load ACL from file's extended attributes
int load_acl(const char* file, ACL* acl) {
    ssize_t acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", NULL, 0);
    if (acl_attr == -1) {
        printf("No ACL found for %s\n", file);
        return -1;
    }

    char* acl_data = (char*)malloc(acl_attr + 1);
    acl_attr = extattr_get_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_data, acl_attr);
    if (acl_attr == -1) {
        printf("Error reading ACL for %s\n", file);
        free(acl_data);
        return -1;
    }

    acl_data[acl_attr] = '\0';  // Null-terminate the string
    deserialize_acl(acl, acl_data);
    free(acl_data);

    return 0;
}

// Save ACL to file's extended attributes
int save_acl(const char* file, ACL* acl) {
    char* acl_str = serialize_acl(acl);

    if (extattr_set_file(file, EXTATTR_NAMESPACE_USER, "acl", acl_str, strlen(acl_str)) == -1) {
        printf("Error: Could not set ACL for %s\n", file);
        free(acl_str);
        return -1;
    }

    free(acl_str);
    return 0;
}

// Example function to set permissions for a user
int set_acl_for_user(ACL* acl, uid_t user, int perms) {
    for (int i = 0; i < acl->entry_count; i++) {
        if (acl->entries[i].user == user) {
            acl->entries[i].perms = perms;  // Update existing ACL entry
            return 0;
        }
    }

    if (acl->entry_count < 10) {
        acl->entries[acl->entry_count].user = user;
        acl->entries[acl->entry_count].perms = perms;
        acl->entry_count++;
        return 0;
    }

    return -1;  // Too many entries
}

// Example function to check ACL for a user
int check_acl(ACL* acl, uid_t user, int perms) {
    for (int i = 0; i < acl->entry_count; i++) {
        if (acl->entries[i].user == user) {
            return acl->entries[i].perms & perms;  // Return permission bitmask
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <file> <user>\n", argv[0]);
        return 1;
    }

    const char* file = argv[1];
    uid_t user = atoi(argv[2]);  // Convert user (ID) passed as argument

    ACL acl;
    acl.entry_count = 0;  // Initialize ACL entry count

    // Load the ACL for the file
    if (load_acl(file, &acl) == -1) {
        return 1;
    }

    // Check if the user has read permission
    if (check_acl(&acl, user, 4)) {
        printf("User %d has read permission for %s\n", user, file);
    } else {
        printf("User %d does not have read permission for %s\n", user, file);
    }

    // Example: set write permission (2) for the user
    if (set_acl_for_user(&acl, user, 6) == 0) {
        printf("Updated ACL for user %d on %s\n", user, file);
        // Save the ACL back to the file
        save_acl(file, &acl);
    } else {
        printf("Failed to update ACL for user %d on %s\n", user, file);
    }

    return 0;
}
