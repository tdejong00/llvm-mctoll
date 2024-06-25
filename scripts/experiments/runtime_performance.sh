# Define benchmarks and corresponding input arguments
benchmarks=("matrix_multiply" "string_match" "word_count" "pca")
arguments=(
	"256 123" # [size] [seed]
	"semper"  # [key]
	"10"      # [top no. of words]
	""
)

REPEATS=100

for ((i = 0; i < ${#benchmarks[@]}; i++)); do
	benchmark=${benchmarks[$i]}
	args=${arguments[$i]}

	for suffix in "" "-opt" "-raised" "-raised-opt"; do
		variant=${benchmark}${suffix}
		target=scripts/build/${variant}

		ls -al $target
		if command -v perf &>/dev/null; then
			sudo perf stat -r $REPEATS $target $args >/dev/null
		else
			for i in $(seq 1 $REPEATS); do
				time $target $args >/dev/null
			done
		fi
	done
done
