// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: echo "test" > tmp
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll tmp | FileCheck %s
// CHECK: no. arguments: 2
// CHECK: argv[1]: tmp
// CHECK: successfully opened file
// CHECK: Size: 5 bytes
// CHECK: successfully read file status
// CHECK: successfully closed file

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("no. arguments: %d\n", argc);
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    printf("argv[1]: %s\n", filename);
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    puts("successfully opened file");

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file status");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Size: %ld bytes\n", file_stat.st_size);

    puts("successfully read file status");

    close(fd);

    puts("successfully closed file");

    return EXIT_SUCCESS;
}
