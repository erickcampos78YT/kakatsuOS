#include <stdint.h>
#include <stddef.h>
#include "vfs.h"

#define MAX_FILESYSTEMS 10
#define MAX_MOUNTPOINTS 20
#define MAX_OPEN_FILES 128

// Estruturas para o VFS
typedef struct filesystem {
    char name[32];
    // Operações do sistema de arquivos
    int (*mount)(struct filesystem *fs, const char *device, const char *mountpoint);
    int (*unmount)(const char *mountpoint);
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void *buffer, size_t size);
    int (*write)(int fd, const void *buffer, size_t size);
    int (*seek)(int fd, int offset, int whence);
    int (*stat)(const char *path, struct stat *st);
    int (*mkdir)(const char *path, int mode);
} filesystem_t;

typedef struct mountpoint {
    char path[256];
    char device[64];
    filesystem_t *fs;
    void *fs_data;
    int mounted;
} mountpoint_t;

typedef struct file {
    int used;
    char path[256];
    mountpoint_t *mount;
    void *fs_data;
    uint32_t position;
    uint32_t flags;
} file_t;

// Tabelas globais
static filesystem_t filesystems[MAX_FILESYSTEMS];
static mountpoint_t mountpoints[MAX_MOUNTPOINTS];
static file_t open_files[MAX_OPEN_FILES];

// Inicializa o VFS
void vfs_init() {
    // Limpar tabelas
    for(int i = 0; i < MAX_FILESYSTEMS; i++) {
        filesystems[i].name[0] = '\0';
    }
    
    for(int i = 0; i < MAX_MOUNTPOINTS; i++) {
        mountpoints[i].mounted = 0;
    }
    
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].used = 0;
    }
    
    // Registrar sistemas de arquivos padrão
    vfs_register_filesystem(&ramfs_operations);
    
    // Montar sistema de arquivos raiz
    vfs_mount("ramfs", NULL, "/");
}

// Registra um sistema de arquivos
int vfs_register_filesystem(filesystem_t *fs) {
    for(int i = 0; i < MAX_FILESYSTEMS; i++) {
        if(filesystems[i].name[0] == '\0') {
            // Copiar operações
            filesystems[i] = *fs;
            return 0;
        }
    }
    return -1; // Sem slots disponíveis
}

// Monta um sistema de arquivos
int vfs_mount(const char *fs_name, const char *device, const char *mountpoint) {
    // Encontrar sistema de arquivos
    filesystem_t *fs = NULL;
    for(int i = 0; i < MAX_FILESYSTEMS; i++) {
        if(strcmp(filesystems[i].name, fs_name) == 0) {
            fs = &filesystems[i];
            break;
        }
    }
    
    if(!fs) {
        return -1; // Sistema de arquivos não encontrado
    }
    
    // Encontrar slot de montagem livre
    int mount_idx = -1;
    for(int i = 0; i < MAX_MOUNTPOINTS; i++) {
        if(!mountpoints[i].mounted) {
            mount_idx = i;
            break;
        }
    }
    
    if(mount_idx == -1) {
        return -1; // Sem slots disponíveis
    }
    
    // Configurar ponto de montagem
    strcpy(mountpoints[mount_idx].path, mountpoint);
    if(device) {
        strcpy(mountpoints[mount_idx].device, device);
    } else {
        mountpoints[mount_idx].device[0] = '\0';
    }
    
    mountpoints[mount_idx].fs = fs;
    mountpoints[mount_idx].mounted = 1;
    
    // Chamar operação de montagem do sistema de arquivos
    if(fs->mount) {
        return fs->mount(fs, device, mountpoint);
    }
    
    return 0;
}

// Desmonta um sistema de arquivos
int vfs_unmount(const char *mountpoint) {
    // Encontrar ponto de montagem
    for(int i = 0; i < MAX_MOUNTPOINTS; i++) {
        if(mountpoints[i].mounted && strcmp(mountpoints[i].path, mountpoint) == 0) {
            // Chamar operação de desmontagem do sistema de arquivos
            if(mountpoints[i].fs->unmount) {
                int result = mountpoints[i].fs->unmount(mountpoint);
                if(result != 0) {
                    return result;
                }
            }
            
            // Limpar ponto de montagem
            mountpoints[i].mounted = 0;
            return 0;
        }
    }
    
    return -1; // Ponto de montagem não encontrado
}

// Encontra o ponto de montagem para um caminho
mountpoint_t *vfs_find_mountpoint(const char *path) {
    mountpoint_t *best_match = NULL;
    size_t best_match_len = 0;
    
    for(int i = 0; i < MAX_MOUNTPOINTS; i++) {
        if(mountpoints[i].mounted) {
            size_t mount_len = strlen(mountpoints[i].path);
            
            // Verificar se este ponto de montagem é um prefixo do caminho
            if(strncmp(path, mountpoints[i].path, mount_len) == 0) {
                // Verificar se é o melhor match até agora
                if(mount_len > best_match_len) {
                    best_match = &mountpoints[i];
                    best_match_len = mount_len;
                }
            }
        }
    }
    
    return best_match;
}

// Abre um arquivo
int vfs_open(const char *path, int flags) {
    // Encontrar ponto de montagem
    mountpoint_t *mount = vfs_find_mountpoint(path);
    if(!mount) {
        return -1; // Caminho não montado
    }
    
    // Calcular caminho relativo ao ponto de montagem
    const char *rel_path = path + strlen(mount->path);
    if(*rel_path == '/') rel_path++; // Pular barra inicial
    
    // Encontrar slot de arquivo livre
    int fd = -1;
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(!open_files[i].used) {
            fd = i;
            break;
        }
    }
    
    if(fd == -1) {
        return -1; // Sem slots disponíveis
    }
    
    // Chamar operação de abertura do sistema de arquivos
    int fs_fd = -1;
    if(mount->fs->open) {
        fs_fd = mount->fs->open(rel_path, flags);
        if(fs_fd < 0) {
            return fs_fd;
        }
    }
    
    // Configurar arquivo aberto
    open_files[fd].used = 1;
    strcpy(open_files[fd].path, path);
    open_files[fd].mount = mount;
    open_files[fd].fs_data = (void*)(intptr_t)fs_fd;
    open_files[fd].position = 0;
    open_files[fd].flags = flags;
    
    return fd;
}

// Lê de um arquivo
int vfs_read(int fd, void *buffer, size_t size) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    // Chamar operação de leitura do sistema de arquivos
    if(open_files[fd].mount->fs->read) {
        int bytes_read = open_files[fd].mount->fs->read(
            (int)(intptr_t)open_files[fd].fs_data, 
            buffer, 
            size
        );
        
        if(bytes_read > 0) {
            open_files[fd].position += bytes_read;
        }
        
        return bytes_read;
    }
    
    return -1;
}

// Escreve em um arquivo
int vfs_write(int fd, const void *buffer, size_t size) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    // Chamar operação de escrita do sistema de arquivos
    if(open_files[fd].mount->fs->write) {
        int bytes_written = open_files[fd].mount->fs->write(
            (int)(intptr_t)open_files[fd].fs_data, 
            buffer, 
            size
        );
        
        if(bytes_written > 0) {
            open_files[fd].position += bytes_written;
        }
        
        return bytes_written;
    }
    
    return -1;
}

// Fecha um arquivo
int vfs_close(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) {
        return -1; // Descritor de arquivo inválido
    }
    
    // Chamar operação de fechamento do sistema de arquivos
    if(open_files[fd].mount->fs->close) {
        int result = open_files[fd].mount->fs->close(
            (int)(intptr_t)open_files[fd].fs_data
        );
        
        if(result != 0) {
            return result;
        }
    }
    
    // Limpar slot de arquivo
    open_files[fd].used = 0;
    return 0;
}
