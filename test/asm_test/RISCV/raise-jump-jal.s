# NOTE: failing because MCTOLL does not seem to support JALs with zero link
#       register (i.e. just jumps) for CFG creation: these instructions do
#       not end the basic block and target does not start a basic block

# UNSUPPORTED: non-functional
# REQUIRES: system-linux
# REQUIRES: riscv64-linux-gnu-gcc
# RUN: riscv64-linux-gnu-gcc -o %t %s
# RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
# RUN: lli %t-dis.ll | FileCheck %s
# CHECK: 0
# CHECK: 1
# CHECK: 2
# CHECK: 3
# CHECK: 4
# CHECK: 5

	.file	"raise-jump.c"
	.option pic
	.text
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
	li	a5,5
	sw	a5,-20(s0)
	sw	zero,-24(s0)
.L2:
	lw	a5,-24(s0)
	mv	a1,a5
	lla	a0,.LC0
	call	printf@plt
	lw	a5,-24(s0)
	mv	a4,a5
	lw	a5,-20(s0)
	sext.w	a4,a4
	sext.w	a5,a5
	bge	a4,a5,.L6
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sw	a5,-24(s0)
	jal	zero,.L2
.L6:
	nop
.L4:
	li	a5,0
	mv	a0,a5
	ld	ra,24(sp)
	ld	s0,16(sp)
	addi	sp,sp,32
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
