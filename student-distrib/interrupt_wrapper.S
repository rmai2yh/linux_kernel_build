.globl keyboard_wrapper, rtc_wrapper, syscall_wrapper, pit_wrapper

#CALLS KEYBOARD_HANDLER
keyboard_wrapper:
	pushl %eax
	pushl %ecx
	pushl %edx
	call keyboard_handler
	popl %edx
	popl %ecx
	popl %eax
	iret

#CALLS RTC_HANDLER
rtc_wrapper:
	pushl %eax
	pushl %ecx
	pushl %edx
	call rtc_handler
	popl %edx
	popl %ecx
	popl %eax
	iret

#CALLS PIT_HANDLER
pit_wrapper:
	cli
	pushl %eax
	pushl %ecx
	pushl %edx
	
	call pit_handler
	
	popl %edx
	popl %ecx
	popl %eax
	iret

#CALLS KEYBOARD_HANDLER
syscall_wrapper:
	
	cli
	#save registers
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushfl

	# check to make sure sys call number is within 1-10
	cmpl $1, %eax
	jl error
	cmpl $10, %eax
	jg error

	# push arguments onto stack
	pushl %edx
	pushl %ecx
	pushl %ebx

	# make the system call via jump table
	sti

	call *jump_table(, %eax, 4)
	
	cli
	# pop arguments off stack
	popl %ebx
	popl %ecx
	popl %edx

syscall_restore:

	# restore saved registers	
	popfl			
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	# return value is in %eax
	sti
	# return to user program
	iret

error:
	movl $-1, %eax
	jmp syscall_restore


jump_table:
.long 0x0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn




#syscall_handler
#1. save regs
#2. check sys call number
#3. make the system call via jump table
#4. restore regs
#5. iret to return to user program
# eax = system call number
# ebx = first argument
# ecx = second arg
# edx = third arg
# ret value in eax
