#!/bin/bash

type qemu-riscv64 >/dev/null 2>&1 || {
	echo >&2 "$0: error: qemu-riscv64 not installed"
	exit 1
}

export QEMU_LD_PREFIX=$RISCV_SYSROOT_PATH
export QEMU_LOG=nochain,cpu
export QEMU_LOG_FILENAME=qemu.log

# Define benchmarks and corresponding input arguments
benchmarks=("matrix_multiply" "string_match" "word_count" "pca")
arguments=(
	"64 123" # [size] [seed]
	"semper" # [key]
	"5"      # [top no. of words]
	""
)

for ((i = 0; i < ${#benchmarks[@]}; i++)); do
	benchmark=${benchmarks[$i]}
	args=${arguments[$i]}

	# Build benchmark
	scripts/build_tests.sh test/smoke_test/RISCV/benchmarks/$benchmark.c

	# Raise and recompile benchmark
	scripts/run.sh scripts/build/$benchmark

	for suffix in "" "-opt" "-raised" "-raised-opt"; do
		variant=${benchmark}${suffix}
		target=scripts/build/${variant}

		ls -al $target

		# Run QEMU emulator with logging
		qemu-riscv64 $target $args >/dev/null

		grep -c "pc" $QEMU_LOG_FILENAME
	done
done
rm $QEMU_LOG_FILENAME
