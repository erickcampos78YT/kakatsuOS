#include <stdint.h>
#include "io.h"
#include "keyboard.h"
#include "idt.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_COMMAND_PORT 0x64

#define KEYBOARD_IRQ 1

// Mapeamento de scancode para ASCII (layout US)
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Buffer circular para armazenar teclas pressionadas
#define KEYBOARD_BUFFER_SIZE 128
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

// Flags de estado do teclado
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int caps_lock = 0;

// Adiciona um caractere ao buffer
static void keyboard_buffer_put(char c) {
    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    
    // Verificar se o buffer está cheio
    if(next_head != buffer_tail) {
        keyboard_buffer[buffer_head] = c;
        buffer_head = next_head;
    }
}

// Retira um caractere do buffer
char keyboard_buffer_get() {
    // Verificar se o buffer está vazio
    if(buffer_head == buffer_tail) {
        return 0;
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

// Verifica se há caracteres no buffer
int keyboard_buffer_available() {
    return buffer_head != buffer_tail;
}

// Handler de interrupção do teclado
static void keyboard_handler(registers_t *regs) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Verificar se é uma tecla pressionada ou liberada
    if(scancode & 0x80) {
        // Tecla liberada (bit 7 = 1)
        scancode &= 0x7F; // Remover bit 7
        
        // Atualizar flags de estado
        if(scancode == 0x2A || scancode == 0x36) { // Left or Right Shift
            shift_pressed = 0;
        } else if(scancode == 0x1D) { // Ctrl
            ctrl_pressed = 0;
        } else if(scancode == 0x38) { // Alt
            alt_pressed = 0;
        }
    } else {
        // Tecla pressionada
        
        // Atualizar flags de estado
        if(scancode == 0x2A || scancode == 0x36) { // Left or Right Shift
            shift_pressed = 1;
            return;
        } else if(scancode == 0x1D) { // Ctrl
            ctrl_pressed = 1;
            return;
        } else if(scancode == 0x38) { // Alt
            alt_pressed = 1;
            return;
        } else if(scancode == 0x3A) { // Caps Lock
            caps_lock = !caps_lock;
            return;
        }
        
        // Converter scancode para ASCII
        if(scancode < sizeof(scancode_to_ascii)) {
            char c = scancode_to_ascii[scancode];
            
            // Aplicar modificadores
            if(c >= 'a' && c <= 'z') {
                if((shift_pressed && !caps_lock) || (!shift_pressed && caps_lock)) {
                    c = c - 'a' + 'A'; // Converter para maiúscula
                }
            } else if(shift_pressed) {
                // Mapear caracteres especiais com Shift
                switch(c) {
                    case '1': c = '!'; break;
                    case '2': c = '@'; break;
                    case '3': c = '#'; break;
                    case '4': c = '$'; break;
                    case '5': c = '%'; break;
                    case '6': c = '^'; break;
                    case '7': c = '&'; break;
                    case '8': c = '*'; break;
                    case '9': c = '('; break;
                    case '0': c = ')'; break;
                    case '-': c = '_'; break;
                    case '=': c = '+'; break;
                    case '[': c = '{'; break;
                    case ']': c = '}'; break;
                    case '\\': c = '|'; break;
                    case ';': c = ':'; break;
                    case '\'': c = '"'; break;
                    case ',': c = '<'; break;
                    case '.': c = '>'; break;
                    case '/': c = '?'; break;
                    case '`': c = '~'; break;
                }
            }
            
            // Processar combinações de teclas com Ctrl
            if(ctrl_pressed) {
                if(c >= 'a' && c <= 'z') {
                    c = c - 'a' + 1; // Ctrl+A = 1, Ctrl+B = 2, etc.
                }
            }
            
            // Adicionar ao buffer se for um caractere válido
            if(c != 0) {
                keyboard_buffer_put(c);
            }
        }
    }
}

// Inicializa o driver de teclado
void keyboard_init() {
    // Registrar handler de interrupção
    register_interrupt_handler(IRQ(KEYBOARD_IRQ), keyboard_handler);
    
    // Limpar buffer
    buffer_head = buffer_tail = 0;
    
    // Resetar flags
    shift_pressed = ctrl_pressed = alt_pressed = caps_lock = 0;
    
    // Habilitar interrupções do teclado
    pic_unmask_irq(KEYBOARD_IRQ);
}

// Lê uma linha do teclado (bloqueante)
void keyboard_read_line(char *buffer, int max_length) {
    int i = 0;
    char c;
    
    while(i < max_length - 1) {
        // Esperar por um caractere
        while(!keyboard_buffer_available()) {
            asm volatile("hlt");
        }
        
        c = keyboard_buffer_get();
        
        // Enter finaliza a linha
        if(c == '\n') {
            buffer[i++] = c;
            break;
        }
        // Backspace remove o último caractere
        else if(c == '\b') {
            if(i > 0) {
                i--;
                // Atualizar console (apagar caractere)
                console_putchar('\b');
                console_putchar(' ');
                console_putchar('\b');
            }
        }
        // Caracteres normais
        else {
            buffer[i++] = c;
            // Eco para o console
            console_putchar(c);
        }
    }
    
    buffer[i] = '\0';
}
