# REQUIRES: system-linux
# REQUIRES: riscv64-linux-gnu-gcc
# RUN: riscv64-linux-gnu-gcc -o %t %s
# RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
# RUN: lli %t-dis.ll | FileCheck %s
# CHECK: equals zero

	.file	"raise-bnez.c"
	.option pic
	.text
	.section	.rodata
	.align	3
.LC0:
	.string	"equals zero"
	.align	3
.LC1:
	.string	"does not equal zero"
	.text
	.align	1
	.globl	func
	.type	func, @function
func:
	addi	sp,sp,-32
	sd	ra,24(sp)
	sd	s0,16(sp)
	addi	s0,sp,32
	mv	a5,a0
	sw	a5,-20(s0)
	lw	a5,-20(s0)
	sext.w	a5,a5
	bne	a5,zero,.L2
	lla	a0,.LC0
	call	puts@plt
	j	.L4
.L2:
	lla	a0,.LC1
	call	puts@plt
.L4:
	nop
	ld	ra,24(sp)
	ld	s0,16(sp)
	addi	sp,sp,32
	jr	ra
	.size	func, .-func
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-16
	sd	ra,8(sp)
	sd	s0,0(sp)
	addi	s0,sp,16
	li	a0,0
	call	func
	li	a5,0
	mv	a0,a5
	ld	ra,8(sp)
	ld	s0,0(sp)
	addi	sp,sp,16
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
