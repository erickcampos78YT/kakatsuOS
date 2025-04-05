#include "kernel.h"
#include "../drivers/vga.h"
#include "../memory/gdt.h"
#include "idt.h"

void kernel_main(void) {
    // Inicializa GDT
    gdt_init();
    
    // Inicializa IDT
    idt_init();
    
    // Inicializa o terminal VGA
    vga_init();
    
    // Banner de boas-vindas
    vga_writestring("BloodMoon Kernel v0.1\n");
    vga_writestring("Developed by erickcampos78YT\n");
    vga_writestring("----------------------------\n");
    
    // Loop infinito
    for(;;) {
        asm volatile("hlt");
    }
}