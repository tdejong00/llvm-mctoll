# Running the experiments

[RISC-V GNU Compiler Toolchain]: https://github.com/riscv-collab/riscv-gnu-toolchain
[riscv64-lp64d]: https://toolchains.bootlin.com/downloads/releases/toolchains/riscv64-lp64d/tarballs/

## Prerequisites

- A Linux system is required.
- All scripts are (at the moment) only meant to be used for the RISC-V raiser.
- All scripts should be called from the root directory of this repository.
- It is assumed that there exists a symbolic link to the build directory of the LLVM-project.
- For cross-compiling using clang, both a RISC-V root file system and the
  [RISC-V GNU Compiler Toolchain] need to be installed. The [riscv64-lp64d]
  is known to work correctly. Furthermore, the environment variables
  `RISCV_TOOLCHAIN_PATH` and `RISCV_SYSROOT_PATH` need to be set to the
  path of the toolchain and system root respectively.

## Usage

```sh
# 1. Initialize the build files
scripts/init.sh ~/repos/llvm-project/llvm ~/repos/llvm-project/build Debug

# 2a. Build the project
scripts/build.sh

# (Optional) 2b. Build the RISC-V test files
scripts/build_tests.sh

# (Optional) 3b. Run all RISC-V tests
scripts/test.sh

# 3. Raise a specific test file
scripts/raise.sh -debug scripts/build/factorial

# 4. Recompile the raised LLVM IR
scripts/recompile.sh scripts/build/factorial

# Alternatively, use the following command, which does both step 3 and 4
scripts/run.sh [-debug] scripts/build/factorial
```
