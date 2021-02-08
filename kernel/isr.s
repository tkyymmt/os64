.macro	irq_handler lbl, func

.global	\lbl
.extern \func
\lbl:
    push    %rax
    push    %rcx
    push    %rdx
    push    %rbx
    push    %rbp
    push    %rsi
    push    %rdi

    call    \func
    
    pop     %rdi
    pop     %rsi
    pop     %rbp
    pop     %rbx
    pop     %rdx
    pop     %rcx
    pop     %rax
    
    iretq
.endm

irq_handler kbc_handler do_kbc_irg


.global default_handler
default_handler:
    jmp     default_handler