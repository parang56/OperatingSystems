#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int fd;
    int offset;

    if(argc < 4){
        printf(2, "usage: lseek_test <filename> <offset> <string>\n");
        exit();
    }

    // Open file in r/w mode
    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        printf(2, "Error: cannot open file %s\n", argv[1]);
        exit();
    }

    // Print original content
    char buf[512];
    int n = read(fd, buf, sizeof(buf) - 1);
    if(n > 0){
        buf[n] = '\0';
        printf(1, "Before: %s\n", buf);
    }

    // Parse offset
    offset = atoi(argv[2]);

    // Seek given offset
    if(lseek(fd, offset, SEEK_SET) < 0){
        printf(2, "Error: lseek failed\n");
        close(fd);
        exit();
    }

    // Write new string at current position
    if(write(fd, argv[3], strlen(argv[3])) < 0){
        printf(2, "Error: write failed\n");
        close(fd);
        exit();
    }

    // Seek back to the beginning to print the modified content
    if(lseek(fd, 0, SEEK_SET) < 0){
        printf(2, "Error: lseek failed\n");
        close(fd);
        exit();
    }

    // Read and print the modified content
    n = read(fd, buf, sizeof(buf) - 1);
    if(n > 0){
        buf[n] = '\0';
        printf(1, "After: %s\n", buf);
    }

    close(fd);
    exit();
}

