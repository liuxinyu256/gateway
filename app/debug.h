#ifndef DEBUG_H
#define DEBUG_H
#include <stdint.h>

/* 调试串口：UART1 */
void debug_init(void);
void debug_putc(char c);
void debug_puts(const char *s);

#endif
