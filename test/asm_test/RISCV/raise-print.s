# REQUIRES: system-linux
# REQUIRES: riscv64-unknown-linux-gnu-gcc
# RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
# RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
# RUN: lli %t-dis.ll | FileCheck %s
# CHECK: Hello, World!
# CHECK: Value: 5

	.file	"raise-print.c"
	.option pic
	.text
	.section	.rodata
	.align	3
.LC0:
	.string	"Hello, World!"
	.align	3
.LC1:
	.string	"Value: %d\n"
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-16
	sd	ra,8(sp)
	sd	s0,0(sp)
	addi	s0,sp,16
	lla	a0,.LC0
	call	puts@plt
	li	a1,5
	lla	a0,.LC1
	call	printf@plt
	li	a5,0
	mv	a0,a5
	ld	ra,8(sp)
	ld	s0,0(sp)
	addi	sp,sp,16
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
