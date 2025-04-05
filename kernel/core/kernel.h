#ifndef KERNEL_H
#define KERNEL_H

void init_vga(void);
void print_char(char ch, int color);
void print_string(const char* str, int color);
void kernel_main(void);

#endif