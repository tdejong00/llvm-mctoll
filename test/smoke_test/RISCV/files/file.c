// NOTE: failing because arrays and structs on stack are not supported yet

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: echo "test" > tmp
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h -I /usr/include/stdlib.h
// RUN: lli %t-dis.ll tmp | FileCheck %s
// CHECK: no. arguments: 2
// CHECK: argv[1]: tmp
// CHECK: successfully opened file
// CHECK: test
// CHECK: successfully read file
// CHECK: successfully closed file

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("no. arguments: %d\n", argc);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    printf("argv[1]: %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    puts("successfully opened file");

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return EXIT_FAILURE;
    }

    puts("successfully read file");

    fclose(file);

    puts("successfully closed file");

    return EXIT_SUCCESS;
}
