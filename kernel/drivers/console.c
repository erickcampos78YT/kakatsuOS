#include <stdint.h>
#include <stddef.h>
#include "console.h"

// Constantes para o modo de texto VGA
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Cores VGA
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Estado do console
static uint16_t *vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t console_color = 0;

// Cria um caractere VGA com cor
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

// Cria um atributo de cor VGA
static inline uint8_t vga_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

// Atualiza a posição do cursor de hardware
static void update_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    // Enviar posição para os registradores do controlador VGA
    outb(0x3D4, 14);                  // Registrador de índice: cursor high
    outb(0x3D5, (pos >> 8) & 0xFF);   // Dados: bits altos da posição
    outb(0x3D4, 15);                  // Registrador de índice: cursor low
    outb(0x3D5, pos & 0xFF);          // Dados: bits baixos da posição
}

// Rola a tela para cima
static void console_scroll() {
    // Mover linhas para cima
    for(int y = 0; y < VGA_HEIGHT - 1; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Limpar última linha
    for(int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', console_color);
    }
    
    // Ajustar cursor
    cursor_y--;
}

// Inicializa o console
void console_init() {
    // Definir cores padrão (texto branco em fundo preto)
    console_color = vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    // Limpar a tela
    for(int y = 0; y < VGA_HEIGHT; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', console_color);
        }
    }
    
    // Inicializar cursor
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

// Define as cores do console
void console_set_color(enum vga_color fg, enum vga_color bg) {
    console_color = vga_color(fg, bg);
}

// Escreve um caractere no console
void console_putchar(char c) {
    // Tratar caracteres especiais
    if(c == '\n') {
        // Nova linha
        cursor_x = 0;
        cursor_y++;
    } else if(c == '\r') {
        // Retorno de carro
        cursor_x = 0;
    } else if(c == '\t') {
        // Tab (4 espaços)
        cursor_x = (cursor_x + 4) & ~3;
    } else if(c == '\b') {
        // Backspace
        if(cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', console_color);
        } else if(cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', console_color);
        }
    } else {
        // Caractere normal
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, console_color);
        cursor_x++;
    }
    
    // Verificar se chegamos ao final da linha
    if(cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    // Verificar se precisamos rolar a tela
    if(cursor_y >= VGA_HEIGHT) {
        console_scroll();
    }
    
    // Atualizar cursor de hardware
    update_cursor();
}

// Escreve uma string no console
void console_write(const char *str) {
    while(*str) {
        console_putchar(*str++);
    }
}

// Limpa o console
void console_clear() {
    for(int y = 0; y < VGA_HEIGHT; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', console_color);
        }
    }
    
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}
