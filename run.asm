section .text
    global __run_interrupt
    global __run_syscall

__run_syscall:
    mov rax, rdi ; move registers from C-abi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    syscall
    ret