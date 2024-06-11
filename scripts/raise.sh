#!/bin/bash
scripts/build.sh || exit 1
build/bin/llvm-mctoll \
  -d \
  -I scripts/includes.h \
  $@ || exit 1
