/* Script de link para KakatsOS */

OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS {
    /* Kernel carregado em 1MB */
    . = 0x100000;

    /* Seção de texto (código) */
    .text : {
        *(.multiboot)   /* Cabeçalho multiboot */
        *(.text)        /* Código */
    }

    /* Seção de dados somente leitura */
    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    /* Seção de dados */
    .data ALIGN(4K) : {
        *(.data)
    }

    /* Seção BSS (dados não inicializados) */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }

    /* Descartar informações de depuração */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
