#ifndef __TEST_H__
#define __TEST_H__

#include "CH57xBLE_LIB.H"
#include "stdint.h"
#define MAXTime 4//帧超时时间

#define UART_EVENT1 0x0001 //计数
#define UART_EVENT2 0x0002 //
#define UART_EVENT3 0x0003

extern  uint8_t TxdataBuff[25];

void UART0_init(void);
void UART1_init(void);
void UART0_TASK_Init(void);
static uint16_t UART0_process_event( uint8_t task_id, uint16_t events );
void TMR0_init(void);
uint8_t InversionPin(uint8_t pin);
void GPIOA6_init(void);
//void GPIO_init(void);

#endif
