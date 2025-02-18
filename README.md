# my_sudo Assignment

## Overview
This program emulates a simplified sudo mechanism using setuid. It checks the target command's ownership and uses seteuid() to run commands with appropriate privileges.

## Build Instructions
1. Run `make` to compile the program.
2. Set ownership and permissions:
   sudo chown root:root my_sudo
   sudo chmod u+s my_sudo

## Testing
- **Root Command Test:**  
  ./my_sudo /usr/bin/whoami  
  Expected: "root"

- **Non-root Command Test:**  
  ./my_sudo ./nonroot.sh  
  Expected: "Effective user: <username>"

## Security Considerations
- The program verifies that the target command exists and is executable.
- It defends against potential race conditions and command injection by validating input and using system calls safely.
