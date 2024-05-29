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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 100

int main(void) {
    int fd = open("tmp", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        return EXIT_FAILURE;
    }

    char *mapped = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    puts(mapped);

    if (munmap(mapped, sb.st_size) == -1) {
        perror("Error unmapping file");
    }

    close(fd);

    return EXIT_SUCCESS;
}
