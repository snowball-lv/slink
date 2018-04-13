

bits 64
; org 0x400000

section .text

global write
write:
    mov rax, 1
    syscall
    ret
