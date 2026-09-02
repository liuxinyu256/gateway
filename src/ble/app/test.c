//#include "CH57x_common.h"
//#include "test.h"
//#include "send.h"



//tmosTaskID Uart0_TaskID;

////volatile uint8_t trigB = 4;
////volatile UINT8 TxBuff[]="This is a tx exam\r\n";

//static UINT8 RxBuff[25]={0};
//uint8 TxdataBuff[25]={0};
//uint8_t cmd;
//volatile uint8_t TxAckFlag=0;		//应答发送标志位
//volatile uint8_t TxtimeFlag=0;	//定时发送标志位
//volatile uint8_t TxCmdFlag=0;		//发送控制命令标志位
//volatile uint8_t timeout=0;			//接收超时时间计数
//volatile uint8_t Txbuff_Len;    //TxBuff中有效数据的长度
//UINT8 index1 =0;
//uint8_t pin=1;


//void UART0_init(void)
//{
//		
//	  GPIOB_SetBits(GPIO_Pin_7);
//    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);			// RXD
//    GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);		// TXD
//		UART0_BaudRateCfg( 9600 );
//		R8_UART0_FCR = (0<<6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN; //FIFO，1字节
//		R8_UART0_LCR = 0x5B;//
//		
//		R8_UART0_DIV = 1;
//		UART0_INTCfg( ENABLE,  RB_IER_RECV_RDY|RB_IER_LINE_STAT);
//		R8_UART0_IER   &= ~(1 << 2);
//		NVIC_EnableIRQ( UART0_IRQn );
//		NVIC_SetPriority(UART0_IRQn,1);
//}

//void UART1_init(void)
//{

//    GPIOA_SetBits(GPIO_Pin_9);
//    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);			// RXD
//    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);		// TXD
//		UART1_BaudRateCfg(115200 );
//		R8_UART1_FCR = (0<<6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;		// FIFO
//    R8_UART1_LCR = RB_LCR_WORD_SZ;	//无校验，8位数据位，1停止位
//    R8_UART1_IER = RB_IER_TXD_EN;	//TXD引脚输出使能 
//    R8_UART1_DIV = 1;	
//		UART1_INTCfg( ENABLE, RB_IER_RECV_RDY|RB_IER_LINE_STAT );
//		NVIC_EnableIRQ(UART1_IRQn);
//		NVIC_SetPriority(UART1_IRQn,3);
//}
//void GPIOA6_init(void)
//{
//	// GPIOA_ResetBits(GPIO_Pin_6);
//    GPIOA_ModeCfg(GPIO_Pin_6, GPIO_ModeOut_PP_5mA);			// RXD
//}
//void TMR0_init(void)
//{
//	
//	TMR0_TimerInit( 16000 );//定时周期500us
//	TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);          // 
//  NVIC_EnableIRQ( TMR0_IRQn );
//	NVIC_SetPriority(TMR0_IRQn ,2);
//	
//}
//void GPIO_init(void)
//{
//	
//	GPIOA_ModeCfg(GPIO_Pin_0, GPIO_ModeOut_PP_5mA);
//	
//} 
//uint8_t InversionPin(uint8_t pin)
//{
//	if(pin==1){
//		 GPIOA_SetBits(GPIO_Pin_6);
//	}else if(pin==0){
//		GPIOA_ResetBits(GPIO_Pin_6);
//	}
//	pin=!pin;
//	return pin;
//}

//uint8_t InversionPinA2(uint8_t pin)
//{
//	if(pin==1){
//		 GPIOA_SetBits(GPIO_Pin_2);
//	}else if(pin==0){
//		GPIOA_ResetBits(GPIO_Pin_2);
//	}
//	pin=!pin;
//	return pin;
//}

//void uart1ControlAC(uint8_t* p2ConnectStep){
//	   //发送控制命令
//					TxCmdFlag=0;
//					if((cmd&0xF0)==0xC0){		//设置温度
//							setTemp(0x10|(cmd&0x0F));
//							cmd=0;
//							}
//						switch(cmd){
//							case 0xA1:	//开机
//								setPower(0x01);
//								cmd=0;
//								break;
//							case 0xA0:	//关机
//								setPower(0x00);
//								cmd=0;
//								break;
//							case 0xB0: 	//制冷
//								setMode(cool);
//								cmd=0;
//								break;
//							case 0xB1: 	//制热
//								setMode(heating);
//								cmd=0;
//								break;
//							case 0xB2: 	//送风
//								setMode(fan);
//								cmd=0;
//								break;
//							case 0xB3:	//除湿
//								setMode(dry);
//								cmd=0;
//								break;
//							case 0xD0:
//								setFanSpeed(AUTO);
//								cmd=0;
//								break;
//							case 0xD1:
//								setFanSpeed(Natural);
//								cmd=0;
//								break;
//							case 0xD2:
//								setFanSpeed(High);
//								cmd=0;
//								break;
//							case 0xD3:
//								setFanSpeed(Mid);
//								cmd=0;
//								break;
//							case 0xD4:
//								setFanSpeed(Low);
//								cmd=0;
//								break;
//							
//						}
//						
//					*p2ConnectStep=4;//保持连接从头开始
//}








////uart0TMOS任务回调函数
//static uint16_t UART0_process_event( uint8_t task_id, uint16_t events )
//{
//	static uint8_t event2Lock=0;
//	static uint8_t state=0;
//	uint8_t *p2ConnectStep=NULL;
//	//static uint8_t event3=0;
//	if(events &UART_EVENT1){
//			//Start_send(TxdataBuff);
//		 if(TxAckFlag==1)	{						//如果接收到数据，发送数据
//				event2Lock=TxAckFlag;
//				TxAckFlag=0;
//				tmos_start_task(Uart0_TaskID,UART_EVENT2,40);//在这25ms延时时间中不能执行TxtimeFlag==2
//			}else if(TxtimeFlag==2){			//未收到数据定时发送数据
//					//uint8_t TxtimeFlag1=TxtimeFlag;
//					if(event2Lock==0&&state!=SearchSuccess){
//							state=Start_Search(TxtimeFlag); 					//在tmos_start_task(Uart0_TaskID,UART_EVENT2,40);25ms期间，需要禁止定时发送数据，因为这里是在发送数据，需要等上一个发送完才能执行，执行到这了冲突了
//					}else if(event2Lock==0&&state==SearchSuccess){	
//							if(TxCmdFlag==1){
//									TxCmdFlag=0;
//									uart1ControlAC(p2ConnectStep);     //发送控制指令
//							}else{
//									p2ConnectStep=KeepConnect(TxtimeFlag);
//							}
//					}
//					TxtimeFlag=0;
//			}	
//	return (events ^ UART_EVENT1);
//}
//	if(events &UART_EVENT2) {
//		
//			if(state!=SearchSuccess){
//						state=Start_Search(event2Lock);
//			}else if(state==SearchSuccess){
//					if(TxCmdFlag==1){
//						 TxCmdFlag=0;
//						 uart1ControlAC(p2ConnectStep);
//					}else{
//						p2ConnectStep=KeepConnect(event2Lock);
//					}
//			}
//			event2Lock=0;
//		return (events ^ UART_EVENT2);
//	}
//	
//	
////	if(events &UART_EVENT3)
////	{
////			Start_Search(TxtimeFlag);
////			//pin=InversionPinA2(pin);
////			

////		return (events ^ UART_EVENT2);
////	}
//	
//	return 0;
//}
//void UART0_TASK_Init(void)
//{
//		Uart0_TaskID=TMOS_ProcessEventRegister(UART0_process_event);
//		tmos_start_reload_task(Uart0_TaskID,UART_EVENT1,1);

//}


//void UART0_IRQHandler(void)
//{
//		
//    switch( UART0_GetITFlag() )
//    {
//			
//        case UART_II_LINE_STAT:        
//            UART0_GetLinSTA();
//            break;
//        
//        case UART_II_RECV_RDY:          // 接收数据可用中断
//							timeout=0;
//							if(index1<25){   					//接收完一个数据，才能接收下一个数据
//							
//								RxBuff[index1] = R8_UART0_RBR ;
//								index1++;
//								
//							}
//						else{
//								R8_UART0_RBR;		        //清中断标志位	
//							}
//            break;
//        
//        case UART_II_RECV_TOUT:         
//	
//    
//            break;
//        
//        case UART_II_THR_EMPTY:         
//							
//				
//            break;
//        
//        case UART_II_MODEM_CHG:         
//            break;
//        
//        default:
//            break;
//    }
//}
//	


//void TMR0_IRQHandler(void)								
//{
//	 if( TMR0_GetITFlag( TMR0_3_IT_CYC_END ) )
//    {
//        TMR0_ClearITFlag( TMR0_3_IT_CYC_END );// 清除中断标志位
//				TxCount++;
//				timeout++;										//先计数，只要进入了定时中断就说明时间走了500us；
//				if(timeout>MAXTime&&index1>0) //接收到数据才会执行，只有接收到数据index1才会大于0，没有接收到数据index=0
//				{
//						
//					if(RxBuff[0]!=0x80&&RxBuff[0]!=0x00){   //0x80和0x00开头的数据是网关发送的，不接收
//							Txbuff_Len=index1;
//							memcpy(TxdataBuff,RxBuff,Txbuff_Len);	//立即将数据拷贝到发送缓冲区
//							TxAckFlag=1;								//表示一帧接收完成
//							index1=0;										//接收数组索引置零，允许下次接收
//							timeout=0;	 								//清空计数值
//							TxCount=0;									//这里清空TxCount意思是，因为收到了一条空调回复的数据，所以下一条发送的话需要重新计时，
//						}else{
//							TxAckFlag=0;
//							timeout=0;
//							index1=0;
//						}
//				}else if(timeout>MAXTime&&index1==0&&TxCount>220){				//如果没收到数据就定时110ms左右发送
//						TxtimeFlag=2;
//						TxCount=0;			
//				}
//							
//							
//    }

//}

//void UART1_IRQHandler(void)
//{		
////		static uint8_t i=1;
//		//static	uint8_t count=0;
//    switch( UART1_GetITFlag() )
//    {
//			
//        case UART_II_LINE_STAT:        //
//            UART1_GetLinSTA();
//            break;
//        
//        case UART_II_RECV_RDY:          //接收
//						cmd=R8_UART1_RBR;
//						TxCmdFlag=1;
//            break;
//        
//        case UART_II_RECV_TOUT:         //接收FIFO超时
//				
//    
//            break;																			
//																				/**UART_II_THR_EMPTY 是指当前发送 FIFO 空。当读取 IIR 寄存器后，该中断被清除，或者当向 THR 写入下一个数据后，该中断也能被清除。 
//																				THR寄存器空，写THR清中断*/
//        case UART_II_THR_EMPTY: 
//							//R8_UART1_IIR;
////							//pin=InversionPin(pin);
////							if(i<Txbuff_Len){	
////								//pin=InversionPin(pin);
////								R8_UART1_THR=TxBuff[i];
////								
////								i++;
////							}else { //i=Txbuff_Len，发送完成
////								i=1;
////							}
//					//	pin=InversionPinA2(pin);
////						R8_UART1_THR=Sendstate;
//            break;
//        
//        case UART_II_MODEM_CHG:         // ???????0
//            break;
//        
//        default:
//            break;
//    }
//}
