/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2019/11/05
 * Description        : 外设从机应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "CH57x_common.h"
#include "HAL.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "TEST.h"
#include "send.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__align(4) u32 MEM_BUF[BLE_MEMHEAP_SIZE/4];

#if (defined (BLE_MAC)) && (BLE_MAC == TRUE)
u8C MacAddr[6] = {0x84,0xC2,0xE4,0x03,0x02,0x02};
#endif

/*******************************************************************************
* Function Name  : main
* Description    : 主函数
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main( void ) 
{
#if (defined (HAL_SLEEP)) && (HAL_SLEEP == TRUE)
  GPIOA_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
  GPIOB_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
#endif
//#ifdef DEBUG
//  GPIOA_SetBits(bTXD1);
//  GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
//  UART1_DefInit( );
//#endif 
//	GPIOA_SetBits(GPIO_Pin_9);
//  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);			// RXD-配置上拉输入
//  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);		// TXD-配置推挽输出，注意先让IO口输出高电平
//  UART1_DefInit();
		
  //PRINT("%s\n",VER_LIB);
  CH57X_BLEInit( );
	HAL_Init( );
	GAPRole_PeripheralInit( );
	Peripheral_Init( ); 
//	//UART0_init();
//	//UART0_TASK_Init();
	GPIOB_ModeCfg(GPIO_Pin_20, GPIO_ModeOut_PP_5mA);	
	GPIOB_ResetBits(GPIO_Pin_20);
//	GPIOB_SetBits(GPIO_Pin_7);
//	GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
	//TMR0_init();
	//GPIOA6_init();
	 //GPIO_init();
	// GPIOA2_init();
//	HbsSet();

	while(1){
		TMOS_SystemProcess( );
	}
}

/******************************** endfile @ main ******************************/
