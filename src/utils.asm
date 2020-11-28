section .data
    integerString db 11,0
    minus db '-',0,0

section .text
global print_integer
global read_integer

read_integer:
    push rbp
    mov rbp,rsp
    sub rsp,5
    mov rsi,integerString
    push rax
    mov rax, 0
    mov rdi, 0
    mov rdx, 11
    syscall

    mov dword[rbp-4],0
    sub al,2
    mov byte [rbp-5],al
    mov rcx,1
    
    _loop:
    mov rbx, integerString
    add bl,byte [rbp-5]
    mov dl,byte [rbx]
    cmp dl,'-' 
    jne _L1
    xor rax,rax
    mov eax,dword [rbp-4]
    mov rcx,-1
    imul rcx
    jmp _L2
    _L1 :
    sub rdx, 48
    mov rax,rdx
    imul rcx
    add dword [rbp-4],eax
    xor rbx,rbx
    mov bl,byte [rbp-5]
    sub rbx,1
    mov byte [rbp-5],bl
    mov rax,10
    imul rcx
    mov rcx,rax
    cmp bl,-1
    jne _loop
    mov eax,dword [rbp-4]
    _L2:
    mov rsp,rbp
    pop rbp
    ret

print_integer :
    push rbp
    mov rbp, rsp
    sub rsp, 5 
    cmp rax,0
    je _L3
    jg _L3
    mov rcx,-1
    imul rcx
    mov dword [rbp-4], eax
    mov rsi,minus
    mov rax,1
    mov rdi,0
    mov rdx,1
    syscall
    
    jmp _L4
    _L3:
    mov dword [rbp-4], eax
    _L4:
    mov eax, [rbp-4]
    mov byte [rbp-5],0
    mov rsi, integerString
    
    _loop1:
    mov rcx,10
    xor rdx,rdx
    idiv rcx
    add dl,'0'
    mov byte [rsi],dl
    add rsi,1
    add byte [rbp-5],1
    cmp rax,0
    jne _loop1

    _loop2:
    sub byte [rbp-5],1
    mov rcx,integerString
    add cl,byte [rbp-5]
    mov rsi,rcx
    mov rax,1
    mov rdi,0
    mov rdx,1
    syscall
    mov rax,[rbp-5]
    cmp byte [rbp-5],0
    jne _loop2

    mov rsp,rbp
    pop rbp
    ret