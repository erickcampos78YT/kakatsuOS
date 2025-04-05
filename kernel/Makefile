# Makefile para KakatsOS

# Compiladores e ferramentas
CC = gcc
AS = nasm
LD = ld
QEMU = qemu-system-i386

# Flags de compilação
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
-Wall -Wextra -Werror -c -I./include
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T link.ld

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = $(KERNEL_DIR)/drivers
MM_DIR = $(KERNEL_DIR)/mm
FS_DIR = $(KERNEL_DIR)/fs
PROC_DIR = $(KERNEL_DIR)/proc

# Arquivos fonte
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_C_SRC = $(wildcard $(KERNEL_DIR)/*.c) \
$(wildcard $(DRIVERS_DIR)/*.c) \
$(wildcard $(MM_DIR)/*.c) \
$(wildcard $(FS_DIR)/*.c) \
$(wildcard $(PROC_DIR)/*.c)
KERNEL_ASM_SRC = $(wildcard $(KERNEL_DIR)/*.asm)

# Arquivos objeto
BOOT_OBJ = $(BUILD_DIR)/boot.bin
KERNEL_C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRC))
KERNEL_ASM_OBJ = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRC))
KERNEL_OBJ = $(KERNEL_C_OBJ) $(KERNEL_ASM_OBJ)

# Imagem de disco
OS_IMAGE = $(BUILD_DIR)/kakatsos.img

# Alvos padrão
.PHONY: all clean run debug

all: $(OS_IMAGE)

# Criar diretórios de build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
	mkdir -p $(BUILD_DIR)/$(MM_DIR)
	mkdir -p $(BUILD_DIR)/$(FS_DIR)
	mkdir -p $(BUILD_DIR)/$(PROC_DIR)

# Compilar bootloader
$(BOOT_OBJ): $(BOOT_SRC) | $(BUILD_DIR)
	$(AS) -f bin $< -o $@

# Compilar arquivos C do kernel
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compilar arquivos ASM do kernel
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Linkar kernel
$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Criar imagem de disco
$(OS_IMAGE): $(BOOT_OBJ) $(BUILD_DIR)/kernel.bin
	# Criar imagem de disco vazia de 10MB
	dd if=/dev/zero of=$@ bs=1M count=10
	# Escrever bootloader no primeiro setor
	dd if=$(BOOT_OBJ) of=$@ conv=notrunc
	# Escrever kernel a partir do segundo setor
	dd if=$(BUILD_DIR)/kernel.bin of=$@ seek=1 conv=notrunc

# Executar no QEMU
run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$<

# Executar no QEMU com GDB
debug: $(OS_IMAGE)
	$(QEMU) -s -S -drive format=raw,file=$<

# Limpar arquivos gerados
clean:
	rm -rf $(BUILD_DIR)
