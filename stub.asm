

bits 64
; org 0x400000

extern write

section .text

global _start
_start:
    
    mov rdi, 1
    mov rsi, message
    mov rdx, 14
    call write

    mov rdi, 0
    call exit

exit:
    mov rax, 60
    syscall

section .rodata

message:
    db  `Hello, World!\n`, 0
