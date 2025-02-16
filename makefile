CC = gcc
CFLAGS = -Wall -Werror
TARGET = my_sudo

all: $(TARGET)

$(TARGET): my_sudo.c
    $(CC) $(CFLAGS) -o $(TARGET) my_sudo.c

clean:
    rm -f $(TARGET)