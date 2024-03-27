# REQUIRES: system-linux
# RUN: riscv64-linux-gnu-gcc -o %t %s
# RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
# RUN: lli %t-dis.ll | FileCheck %s
# CHECK: 4

	.file	"raise-ptr-call.c"
	.option pic
	.text
	.align	1
	.globl	increment
	.type	increment, @function
increment:
	addi	sp,sp,-32
	sd	s0,24(sp)
	addi	s0,sp,32
	sd	a0,-24(s0)
	ld	a5,-24(s0)
	lw	a5,0(a5)
	addiw	a5,a5,1
	sext.w	a4,a5
	ld	a5,-24(s0)
	sw	a4,0(a5)
	nop
	ld	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	increment, .-increment
	.section	.rodata
	.align	3
.LC0:
	.string	"%d\n"
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-32
	sd	ra,24(sp)
	sd	s0,16(sp)
	addi	s0,sp,32
	li	a5,3
	sw	a5,-20(s0)
	addi	a5,s0,-20
	mv	a0,a5
	call	increment
	lw	a5,-20(s0)
	mv	a1,a5
	lla	a0,.LC0
	call	printf@plt
	li	a5,0
	mv	a0,a5
	ld	ra,24(sp)
	ld	s0,16(sp)
	addi	sp,sp,32
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
