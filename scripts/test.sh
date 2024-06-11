#!/bin/bash
scripts/build.sh || exit 1
DEFAULT_TEST_PATH="test/*/RISCV"
TEST_PATH=${1:-$DEFAULT_TEST_PATH}
build/bin/llvm-lit $TEST_PATH
