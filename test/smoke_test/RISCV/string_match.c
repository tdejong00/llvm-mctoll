// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/string.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Key found!

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void string_match(char *text, char *key) {
  const char *DELIMITERS = " ,.";

  char *token = strtok(text, DELIMITERS);

  while (token != NULL) {
    if(!strcmp(key, token)) {
        puts("Key found!");
        return;
    }
    token = strtok(NULL, DELIMITERS);
  }
  puts("Key not found.");
}

int main(void) {
  const char *TEXT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. \
                    Maecenas fermentum ex a diam commodo convallis. Vivamus \
                    vehicula sem quis tortor tincidunt, vitae viverra velit \
                    dignissim. Phasellus vitae dolor justo. Praesent vel nisi \
                    urna. Morbi at rhoncus erat, [KEY] non tristique est. Duis \
                    rhoncus ipsum eget purus malesuada, eget tempus lectus \
                    ullamcorper. Proin vehicula maximus odio nec convallis.";

  char *text = (char *)malloc(strlen(TEXT) + 1);
  strcpy(text, TEXT);

  const char *KEY = "[KEY]";
  char *key = (char *)malloc(strlen(KEY) + 1);
  strcpy(key, KEY);

  string_match(text, key);
  return 0;
}
