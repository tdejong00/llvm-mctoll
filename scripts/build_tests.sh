#!/bin/bash

compile_example() {
  local target_dir=$1
  local file=$2

  filename=$(basename -- "$file")
  filename="${filename%.*}"

  local target="${target_dir}/${filename}"

  echo $target

  riscv64-unknown-linux-gnu-gcc -O0 -fno-stack-protector $file -S -o "${target}.s"
  riscv64-unknown-linux-gnu-gcc -O0 -fno-stack-protector "${target}.s" -o "${target}"
  riscv64-unknown-linux-gnu-gcc -O0 -fno-stack-protector -static "${target}.s" -o "${target}-static"
  riscv64-unknown-linux-gnu-gcc -O3 -fno-stack-protector $file -S -o "${target}-opt.s"
  riscv64-unknown-linux-gnu-gcc -O3 -fno-stack-protector "${target}-opt.s" -o "${target}-opt"
  riscv64-unknown-linux-gnu-gcc -O3 -fno-stack-protector -static "${target}-opt.s" -o "${target}-static-opt"
}

compile_examples() {
  local target_dir=$1

  for file in $(find test/*/RISCV -type f -name "*.c"); do
    compile_example "$target_dir" "$file"
  done
}

mkdir -p scripts/build
if [ -n "$1" ]; then
  rm -f scripts/build/$(basename -- "$1" | cut -d. -f1)*
  compile_example "scripts/build" "$1"
else
  rm -rf scripts/build/*
  compile_examples "scripts/build"
fi
