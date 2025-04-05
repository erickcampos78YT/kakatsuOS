#include <stdint.h>
#include "scheduler.h"

#define MAX_PROCESSES 256

// Estrutura para PCB (Process Control Block)
typedef struct {
    uint32_t pid;
    uint32_t esp;     // Stack pointer
    uint32_t ebp;     // Base pointer
    uint32_t eip;     // Instruction pointer
    uint32_t cr3;     // Page directory
    uint8_t state;    // RUNNING, READY, BLOCKED, etc.
    uint8_t priority;
    uint32_t quantum; // Tempo de execução restante
} process_t;

// Lista de processos
static process_t processes[MAX_PROCESSES];
static uint32_t current_process = 0;
static uint32_t next_pid = 1;

// Inicializa o escalonador
void scheduler_init() {
    // Inicializar processo kernel (PID 0)
    processes[0].pid = 0;
    processes[0].state = PROCESS_RUNNING;
    processes[0].priority = 0;
    processes[0].quantum = 10;
    
    // Configurar timer para preempção
    pit_set_frequency(100); // 100Hz = 10ms por tick
    pit_register_handler(scheduler_tick);
}

// Chamado a cada tick do timer
void scheduler_tick() {
    // Decrementar quantum do processo atual
    if(processes[current_process].quantum > 0) {
        processes[current_process].quantum--;
    }
    
    // Se o quantum acabou, fazer preempção
    if(processes[current_process].quantum == 0) {
        scheduler_schedule();
    }
}

// Escolhe o próximo processo a executar
void scheduler_schedule() {
    // Salvar contexto do processo atual
    asm volatile("mov %%esp, %0" : "=r"(processes[current_process].esp));
    asm volatile("mov %%ebp, %0" : "=r"(processes[current_process].ebp));
    
    // Marcar processo atual como pronto
    if(processes[current_process].state == PROCESS_RUNNING) {
        processes[current_process].state = PROCESS_READY;
    }
    
    // Algoritmo Round-Robin simples
    uint32_t next = (current_process + 1) % MAX_PROCESSES;
    while(next != current_process) {
        if(processes[next].state == PROCESS_READY) {
            break;
        }
        next = (next + 1) % MAX_PROCESSES;
    }
    
    // Se não encontrou processo pronto, continua no atual
    if(next == current_process && processes[current_process].state != PROCESS_READY) {
        // Nenhum processo disponível
        return;
    }
    
    // Atualizar processo atual
    current_process = next;
    processes[current_process].state = PROCESS_RUNNING;
    processes[current_process].quantum = 10; // Reset quantum
    
    // Restaurar contexto do novo processo
    uint32_t esp = processes[current_process].esp;
    uint32_t ebp = processes[current_process].ebp;
    uint32_t cr3 = processes[current_process].cr3;
    
    // Trocar diretório de páginas
    if(cr3 != 0) {
        asm volatile("mov %0, %%cr3" : : "r"(cr3));
    }
    
    // Restaurar registradores
    asm volatile("mov %0, %%esp" : : "r"(esp));
    asm volatile("mov %0, %%ebp" : : "r"(ebp));
}

// Cria um novo processo
uint32_t process_create(void *entry_point, uint8_t priority) {
    // Encontrar slot livre
    uint32_t pid = 0;
    for(uint32_t i = 1; i < MAX_PROCESSES; i++) {
        if(processes[i].state == PROCESS_NONE) {
            pid = i;
            break;
        }
    }
    
    if(pid == 0) {
        return 0; // Sem slots disponíveis
    }
    
    // Alocar pilha para o processo
    void *stack = vmm_alloc_pages(2); // 8KB de pilha
    if(!stack) {
        return 0;
    }
    
    // Configurar PCB
    processes[pid].pid = next_pid++;
    processes[pid].esp = (uint32_t)stack + 8192 - 4; // Topo da pilha
    processes[pid].ebp = processes[pid].esp;
    processes[pid].eip = (uint32_t)entry_point;
    processes[pid].state = PROCESS_READY;
    processes[pid].priority = priority;
    processes[pid].quantum = 10;
    
    // Configurar frame inicial na pilha
    uint32_t *stack_ptr = (uint32_t*)processes[pid].esp;
    *stack_ptr = (uint32_t)entry_point; // EIP para retorno
    
    // Criar diretório de páginas para o processo
    processes[pid].cr3 = vmm_create_address_space();
    
    return processes[pid].pid;
}
