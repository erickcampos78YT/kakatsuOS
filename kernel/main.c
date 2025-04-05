#include <stddef.h>
#include <stdint.h>

// Função principal do kernel
void kernel_main() {
    // Inicializar o console para saída básica
    console_init();
    
    console_write("KakatsOS - Kernel inicializado\n");
    
    // Inicializar subsistemas do kernel
    gdt_init();       // Tabela de Descritores Globais
    idt_init();       // Tabela de Descritores de Interrupção
    pmm_init();       // Gerenciador de Memória Física
    vmm_init();       // Gerenciador de Memória Virtual
    
    // Inicializar escalonador
    scheduler_init();
    
    // Inicializar sistema de arquivos
    vfs_init();
    
    // Loop infinito para manter o kernel ativo
    while(1) {
        // Habilitar interrupções
        asm volatile("sti");
        // Colocar CPU em estado de baixo consumo até próxima interrupção
        asm volatile("hlt");
    }
}
