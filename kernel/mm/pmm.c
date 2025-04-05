#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

// Bitmap para rastrear páginas físicas (1 = usado, 0 = livre)
static uint32_t *physical_memory_bitmap;
static uint32_t total_pages;
static uint32_t used_pages;

// Inicializa o gerenciador de memória física
void pmm_init(multiboot_info_t *mbi) {
    // Obter informações de memória do bootloader
    memory_map_t *mmap = (memory_map_t*)mbi->mmap_addr;
    uint32_t total_memory = 0;
    
    // Calcular memória total disponível
    while((unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if(mmap->type == 1) { // Memória disponível
            total_memory += mmap->length_low;
        }
        mmap = (memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
    }
    
    // Calcular número total de páginas (4KB por página)
    total_pages = total_memory / 4096;
    used_pages = 0;
    
    // Alocar bitmap para rastrear páginas
    physical_memory_bitmap = (uint32_t*)BITMAP_ADDRESS;
    
    // Inicializar bitmap (0 = livre)
    for(uint32_t i = 0; i < total_pages / 32; i++) {
        physical_memory_bitmap[i] = 0;
    }
    
    // Marcar páginas de baixa memória e do kernel como usadas
    pmm_mark_region_used(0, KERNEL_END_ADDRESS / 4096);
}

// Aloca uma página física
void* pmm_alloc_page() {
    if(used_pages >= total_pages) {
        return NULL; // Sem memória disponível
    }
    
    // Encontrar uma página livre
    for(uint32_t i = 0; i < total_pages / 32; i++) {
        if(physical_memory_bitmap[i] != 0xFFFFFFFF) {
            // Encontrar bit livre
            for(uint8_t j = 0; j < 32; j++) {
                uint32_t bit = 1 << j;
                if(!(physical_memory_bitmap[i] & bit)) {
                    // Marcar como usado
                    physical_memory_bitmap[i] |= bit;
                    used_pages++;
                    
                    // Calcular endereço físico
                    uint32_t page = i * 32 + j;
                    return (void*)(page * 4096);
                }
            }
        }
    }
    
    return NULL; // Não deveria chegar aqui
}

// Libera uma página física
void pmm_free_page(void *page_addr) {
    uint32_t page = (uint32_t)page_addr / 4096;
    uint32_t index = page / 32;
    uint32_t bit = 1 << (page % 32);
    
    // Verificar se a página está realmente alocada
    if(physical_memory_bitmap[index] & bit) {
        physical_memory_bitmap[index] &= ~bit;
        used_pages--;
    }
}
