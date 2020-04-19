#define ASM     1
.global sys_calls,invalid,sys_call_done,sys_call_table
sys_calls:
    # save all reg
    pushl   %esi
    pushl   %edi
    pushl   %esp

    # pushl all the para

    pushl   %edx
    pushl   %ecx
    pushl   %ebx

    # check whether the sys call number is valid
    cmp     $1, %eax
    jl invalid
    cmp     $10, %eax
    jg invalid

    # If the sys call number is valid, call the function
    call *sys_call_table(,%eax,4)
    jmp sys_call_done

invalid:
    movl    $-1, %eax

sys_call_done:
    # popl all the registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esp
    popl %edi
    popl %esi
    
    # need to check sig :
    # call 

    iret

# The sys call table
sys_call_table:
    .long   0x0
    .long   halt
    .long   execute
    .long   read
    .long   write
    .long   open
    .long   close
    .long   getargs
    .long   vidmap
    .long   set_handler
    .long   sigreturn

