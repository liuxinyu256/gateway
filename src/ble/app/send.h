#ifndef __SEND_H__
#define __SEND_H__

#include "stdint.h"
#include "CH57xBLE_LIB.H"
#define SearchSuccess 1
#define SearchContinue   2



void  Open_TMR1(void);
void Start_send(uint8_t* TxBuff);
extern uint8_t Curren_Step;
extern uint16_t TxCount;
uint8_t Start_Search(uint8_t flag);
void send_GroupEnd(uint8_t* GroupCount);

typedef enum
{
	IDIE=0,						//空闲状态没有发送数据		0
	START_BIT_LOW,		//											1
  START_BIT_HIGH,		//起始位，高电平					2
  DATA_BIT_LOW,			//数据位低电平						3
  DATA_BIT_HIGH,		//数据位高电平						4
  PARITY_BIT_LOW,		//校验位低电平						5
  PARITY_BIT_HIGH,	//校验位高电平						6
	STOP_BIT_LOW,			//											7
  STOP_BIT_HIGH,			//停止位高电平						8
}SendState;







#endif


