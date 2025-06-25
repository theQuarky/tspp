	.text
	.file	"tspp_module"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	xorl	%eax, %eax
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	globalCounter,@object           # @globalCounter
	.bss
	.globl	globalCounter
	.p2align	2, 0x0
globalCounter:
	.long	0                               # 0x0
	.size	globalCounter, 4

	.section	".note.GNU-stack","",@progbits
