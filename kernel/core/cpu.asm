; cpu.asm - Funções de baixo nível para CPU

global gdt_flush     ; Permite que C chame gdt_flush()
global idt_load      ; Permite que C chame idt_load()

gdt_flush:
    mov eax, [esp+4]  ; Pega o ponteiro do parâmetro
    lgdt [eax]        ; Carrega a nova GDT

    mov ax, 0x10      ; 0x10 é o offset no GDT para nosso segmento de dados
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 é o offset para o segmento de código
.flush:
    ret

idt_load:
    mov eax, [esp+4]  ; Pega o ponteiro do parâmetro
    lidt [eax]        ; Carrega a IDT
    ret