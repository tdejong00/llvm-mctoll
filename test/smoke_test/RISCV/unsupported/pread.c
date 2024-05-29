// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: echo "test" > tmp
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h,/usr/include/fcntl.h,/usr/include/unistd.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: test
// XFAIL: *

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 100

int main(void) {
    int fd = open("tmp", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read = pread(fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read < 0) {
        perror("Error reading file");
        close(fd);
        return EXIT_FAILURE;
    }

    buffer[bytes_read] = '\0';
    puts(buffer);

    close(fd);

    return EXIT_SUCCESS;
}
