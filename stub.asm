

bits 64
org 0x400000


_start:
    
    mov rdi, 1
    mov rsi, message
    mov rdx, 14
    call write

    mov rdi, 0
    call exit

write:
    mov rax, 1
    syscall

exit:
    mov rax, 60
    syscall

message:
    db  `Hello, World!\n`, 0
