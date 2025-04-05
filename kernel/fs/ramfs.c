#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "vfs.h"
#include "ramfs.h"

#define RAMFS_MAX_FILES 64
#define RAMFS_MAX_FILENAME 64
#define RAMFS_MAX_SIZE (1024 * 1024) // 1MB por arquivo

// Estrutura para um arquivo no RAMFS
typedef struct ramfs_file {
    char name[RAMFS_MAX_FILENAME];
    uint8_t *data;
    size_t size;
    size_t allocated;
    int used;
} ramfs_file_t;

// Estrutura para o sistema de arquivos RAMFS
typedef struct ramfs_data {
    ramfs_file_t files[RAMFS_MAX_FILES];
} ramfs_data_t;

// Instância global do RAMFS
static ramfs_data_t ramfs;

// Inicializa o RAMFS
static int ramfs_mount(struct filesystem *fs, const char *device, const char *mountpoint) {
    // Limpar tabela de arquivos
    for(int i = 0; i < RAMFS_MAX_FILES; i++) {
        ramfs.files[i].used = 0;
        ramfs.files[i].data = NULL;
        ramfs.files[i].size = 0;
        ramfs.files[i].allocated = 0;
    }
    
    return 0;
}

// Desmonta o RAMFS
static int ramfs_unmount(const char *mountpoint) {
    // Liberar memória de todos os arquivos
    for(int i = 0; i < RAMFS_MAX_FILES; i++) {
        if(ramfs.files[i].used && ramfs.files[i].data) {
            free(ramfs.files[i].data);
            ramfs.files[i].data = NULL;
            ramfs.files[i].used = 0;
        }
    }
    
    return 0;
}

// Encontra um arquivo pelo nome
static ramfs_file_t *ramfs_find_file(const char *path) {
    for(int i = 0; i < RAMFS_MAX_FILES; i++) {
        if(ramfs.files[i].used && strcmp(ramfs.files[i].name, path) == 0) {
            return &ramfs.files[i];
        }
    }
    
    return NULL;
}

// Abre um arquivo no RAMFS
static int ramfs_open(const char *path, int flags) {
    ramfs_file_t *file = ramfs_find_file(path);
    
    // Se o arquivo não existe e estamos criando
    if(!file && (flags & O_CREAT)) {
        // Encontrar slot livre
        int file_idx = -1;
        for(int i = 0; i < RAMFS_MAX_FILES; i++) {
            if(!ramfs.files[i].used) {
                file_idx = i;
                break;
            }
        }
        
        if(file_idx == -1) {
            return -1; // Sem slots disponíveis
        }
        
        // Criar novo arquivo
        strcpy(ramfs.files[file_idx].name, path);
        ramfs.files[file_idx].size = 0;
        ramfs.files[file_idx].allocated = 0;
        ramfs.files[file_idx].data = NULL;
        ramfs.files[file_idx].used = 1;
        
        return file_idx;
    }
    
    // Se o arquivo não existe e não estamos criando
    if(!file) {
        return -1;
    }
    
    // Encontrar índice do arquivo
    for(int i = 0; i < RAMFS_MAX_FILES; i++) {
        if(&ramfs.files[i] == file) {
            return i;
        }
    }
    
    return -1; // Não deveria chegar aqui
}

// Lê de um arquivo no RAMFS
static int ramfs_read(int fd, void *buffer, size_t size) {
    if(fd < 0 || fd >= RAMFS_MAX_FILES || !ramfs.files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    ramfs_file_t *file = &ramfs.files[fd];
    
    // Verificar se há dados para ler
    if(!file->data || file->size == 0) {
        return 0;
    }
    
    // Calcular quanto podemos ler
    size_t bytes_to_read = size;
    if(bytes_to_read > file->size) {
        bytes_to_read = file->size;
    }
    
    // Copiar dados
    memcpy(buffer, file->data, bytes_to_read);
    
    return bytes_to_read;
}

// Escreve em um arquivo no RAMFS
static int ramfs_write(int fd, const void *buffer, size_t size) {
    if(fd < 0 || fd >= RAMFS_MAX_FILES || !ramfs.files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    ramfs_file_t *file = &ramfs.files[fd];
    
    // Verificar se precisamos alocar mais memória
    if(file->size + size > file->allocated) {
        size_t new_size = file->size + size;
        
        // Arredondar para múltiplo de 4KB
        new_size = (new_size + 4095) & ~4095;
        
        // Limitar tamanho máximo
        if(new_size > RAMFS_MAX_SIZE) {
            new_size = RAMFS_MAX_SIZE;
        }
        
        // Realocar buffer
        uint8_t *new_data = realloc(file->data, new_size);
        if(!new_data) {
            return -1; // Falha na alocação
        }
        
        file->data = new_data;
        file->allocated = new_size;
    }
    
    // Copiar dados
    memcpy(file->data + file->size, buffer, size);
    file->size += size;
    
    return size;
}

// Fecha um arquivo no RAMFS
static int ramfs_close(int fd) {
    if(fd < 0 || fd >= RAMFS_MAX_FILES || !ramfs.files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    // Não precisamos fazer nada especial para fechar
    return 0;
}

// Operações do sistema de arquivos RAMFS
filesystem_t ramfs_operations = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .unmount = ramfs_unmount,
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .seek = NULL,  // Não implementado
    .stat = NULL,  // Não implementado
    .mkdir = NULL  // Não implementado
};
