bits 64


section .text

; 60 - sys_exit: int error_code	
global sys_exit
sys_exit:
    mov rax, 60
    syscall
    ; unreachable
