#!/bin/bash

for benchmark in "matrix_multiply" "string_match" "word_count" "pca"; do
	# Build benchmark
	scripts/build_tests.sh test/smoke_test/RISCV/benchmarks/$benchmark.c

	# Raise and recompile benchmark
	scripts/run.sh scripts/build/$benchmark

	for suffix in "" "-opt" "-raised" "-raised-opt"; do
		target=scripts/build/${benchmark}${suffix}

		# Binary size
		ls -al $target

		# Amount of sections
		num_sections=$($RISCV_TOOLCHAIN_PATH/bin/riscv64-unknown-elf-readelf -S $target | grep '\] \.' | wc -l)
		echo "No. of sections: $num_sections"

		# Section sizes
		$RISCV_TOOLCHAIN_PATH/bin/riscv64-unknown-elf-size --format=sysv $target
		$RISCV_TOOLCHAIN_PATH/bin/riscv64-unknown-elf-size $target
	done
done
