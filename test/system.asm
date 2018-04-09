

extern main

section .text

    ; 1 - sys_write: unsigned int fd, const char *buf, size_t count
    global write
    write:
        mov rax, 1
        syscall
        ret

    global _start
    _start:
        call main
        mov rdi, rax
        call exit
        ; unreachable

    ; 60 - sys_exit: int error_code	
    exit:
        mov rax, 60
        syscall
        ; unreachable
