bits 64


section .text

; 1 - sys_write: unsigned int fd, const char *buf, size_t count
global sys_write
sys_write:
    mov rax, 1
    syscall
    ret
