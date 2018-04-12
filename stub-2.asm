

bits 64
; org 0x400000

section .text

global exit
exit:
    mov rax, 60
    syscall
