; boot.asm - Bootloader for KakatsOS

BITS 16
org 0x7C00

start:
    ; Configura o segmento de dados
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Configura o modo de vídeo
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Mensagem de Boas-vindas
    mov si, welcome_msg
print_char:
    lodsb
    cmp al, 0
    je load_kernel
    mov ah, 0x0E
    int 0x10
    jmp print_char

load_kernel:
    ; Carrega o kernel (assume que está no segundo setor)
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000  ; Carrega o kernel para o endereço 0x8000:0000
    mov ah, 0x02
    mov al, 0x01    ; Número de setores a serem lidos
    mov ch, 0x00    ; Trilha
    mov cl, 0x02    ; Setor (segundo setor)
    mov dh, 0x00    ; Cabeça
    mov dl, 0x80    ; Unidade (primeiro disco)
    int 0x13
    jc disk_error

    ; Salta para o kernel
    jmp 0x8000:0000

disk_error:
    ; Mensagem de erro
    mov si, error_msg
print_error:
    lodsb
    cmp al, 0
    je halt
    mov ah, 0x0E
    int 0x10
    jmp print_error

halt:
    hlt
    jmp halt

welcome_msg db 'KakatsOS Bootloader', 0
error_msg db 'Erro ao carregar o kernel', 0

times 510-($-$$) db 0
dw 0xAA55
