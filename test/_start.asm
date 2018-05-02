bits 64


extern main
extern exit

section .text

global _start
_start:
    call main
    mov rdi, rax
    call exit
    ; unreachable
