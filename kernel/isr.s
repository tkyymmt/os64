.global kbc_handler
.extern do_kbc_irg

kbc_handler:
    push    %rax
    push    %rcx
    push    %rdx
    push    %rbx
    push    %rbp
    push    %rsi
    push    %rdi
    call    do_kbc_irg
    pop     %rdi
    pop     %rsi
    pop     %rbp
    pop     %rbx
    pop     %rdx
    pop     %rcx
    pop     %rax
    iretq


.global default_handler
default_handler:
    jmp     default_handler