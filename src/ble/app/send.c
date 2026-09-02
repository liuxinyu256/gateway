//#include "CH57x_common.h"
//#include "send.h"
//#include "CH57x_gpio.h"
//#include "test.h"
//#include "DAIKIN_CRC.h"
//volatile uint8_t Sendstate=IDIE;				//发送状态

//static uint8_t *TxBuff=NULL;     //发送数组指针
//uint8_t pin1=1;
//uint16_t TxCount=0;					//定时发送计数器
//uint8_t	TxbuffLen;				//需要发送数组的长度
//					
//typedef union {
//	uint8_t	TxBuffDATA[4];
//	struct{

//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//	}data;
//}TxByte_4;

//typedef union {
//	uint8_t	TxBuffDATA[5];
//	struct{
//	
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//	}data;
//}TxByte_5;

//typedef union {
//	uint8_t	TxBuffDATA[8];
//	struct{
//	
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//		uint8_t Byte_5;
//		uint8_t Byte_6;
//		uint8_t Byte_7;
//	}data;
//}TxByte_8;

//typedef union {
//	uint8_t	TxBuffDATA[9];
//	struct{

//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//		uint8_t Byte_5;
//		uint8_t Byte_6;
//		uint8_t Byte_7;
//		uint8_t Byte_8;
//	}data;
//}TxByte_9;

//typedef union {
//	uint8_t	TxBuffDATA[11];
//	struct{
//	
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//		uint8_t Byte_5;
//		uint8_t Byte_6;
//		uint8_t Byte_7;
//		uint8_t Byte_8;
//		uint8_t Byte_9;
//		uint8_t Byte_10;
//	}data;
//}TxByte_11;

//typedef union {
//	uint8_t	TxBuffDATA[12];
//	struct{
//	
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//		uint8_t Byte_5;
//		uint8_t Byte_6;
//		uint8_t Byte_7;
//		uint8_t Byte_8;
//		uint8_t Byte_9;
//		uint8_t Byte_10;
//		uint8_t Byte_11;
//	}data;
//}TxByte_12;

//typedef union {
//	uint8_t	TxBuffDATA[14];
//	struct{
//	
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;
//		uint8_t Byte_4;
//		uint8_t Byte_5;
//		uint8_t Byte_6;
//		uint8_t Byte_7;
//		uint8_t Byte_8;
//		uint8_t Byte_9;
//		uint8_t Byte_10;
//		uint8_t Byte_11;
//		uint8_t Byte_12;
//		uint8_t Byte_13;
//	}data;
//}TxByte_14;
//typedef union {
//	uint8_t	TxBuffDATA[20];
//	struct{
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;	
//		uint8_t Byte_4;	
//		uint8_t Byte_5;	
//		uint8_t Byte_6;		
//		uint8_t Byte_7;	
//		uint8_t Byte_8;	
//		uint8_t Byte_9;	
//		uint8_t Byte_10;
//		uint8_t Byte_11;
//		uint8_t Byte_12;
//		uint8_t Byte_13;
//		uint8_t Byte_14;
//		uint8_t Byte_15;
//		uint8_t Byte_16;
//		uint8_t Byte_17;
//		uint8_t Byte_18;
//		uint8_t Byte_19;
//	}data;
//}TxByte_20;
//typedef union {
//	uint8_t	TxBuffDATA[24];
//	struct{
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t power;	//电源开关机
//		uint8_t modeH;	//模式高字节
//		uint8_t modeL;	//模式低字节
//		uint8_t temp;		//温度
//		uint8_t Byte_7;	//空0x00
//		uint8_t fan;	//风量 
//		uint8_t Byte_9;	//0x00
//		uint8_t FanDirection;//风向 42自动风 02前后左右风
//		uint8_t Byte_11;//
//		uint8_t Byte_12;
//		uint8_t Byte_13;
//		uint8_t Byte_14;
//		uint8_t Byte_15;
//		uint8_t Byte_16;
//		uint8_t Byte_17;
//		uint8_t Byte_18;
//		uint8_t Byte_19;
//		uint8_t Byte_20;
//		uint8_t Byte_21;
//		uint8_t Byte_22;
//		uint8_t Byte_23;
//	}data;
//}TxStateByte_24;

//typedef union {
//	uint8_t	TxBuffDATA[24];
//	struct{
//		uint8_t Byte_0;
//		uint8_t Byte_1;
//		uint8_t Byte_2;
//		uint8_t Byte_3;	
//		uint8_t Byte_4;	
//		uint8_t Byte_5;	
//		uint8_t Byte_6;		
//		uint8_t Byte_7;	
//		uint8_t Byte_8;	
//		uint8_t Byte_9;	
//		uint8_t Byte_10;
//		uint8_t Byte_11;
//		uint8_t Byte_12;
//		uint8_t Byte_13;
//		uint8_t Byte_14;
//		uint8_t Byte_15;
//		uint8_t Byte_16;
//		uint8_t Byte_17;
//		uint8_t Byte_18;
//		uint8_t Byte_19;
//		uint8_t Byte_20;
//		uint8_t Byte_21;
//		uint8_t Byte_22;
//		uint8_t Byte_23;
//	}data;
//}TxByte_24;

//static TxByte_4 SearchAC_Byte4_data;

//static TxByte_5 SearchAC_Byte5_data;

//static TxByte_8 SearchAC_Byte8_data;

//static TxStateByte_24 Byte24_data;

//static TxByte_24 Rx20_data;  //存储给00 00 20 开头的数据应答的空调的应答数据

//static TxByte_24 Rx21_data;	//存储给00 00 21 开头的数据应答的空调的应答数据

//static TxByte_8 SearchAC_Start3_data;

//static TxByte_12 Byte12_data;

//static TxByte_14 Byte14_data;

//static TxByte_20 Byte20_data;
//	
////0x80 0x80 开头4字节的数据
//void SearchAC_Step1(TxByte_4* data,uint8_t Byte_2){
//		TxbuffLen=4;
//		data->data.Byte_0=0x80;
//		data->data.Byte_1=0x80;
//		data->data.Byte_2=Byte_2;
//		data->data.Byte_3=DAIKIN_CRC_Calc(data->TxBuffDATA,3);
//		Start_send(data->TxBuffDATA);
//		
//}



//void SearchAC_Step2(TxByte_8* data,uint8_t Byte_3,uint8_t Byte_4,uint8_t Byte_5,uint8_t Byte_6){
//		TxbuffLen=8;
//		data->data.Byte_0=0x80;
//		data->data.Byte_1=0x80;
//		data->data.Byte_2=0x03;
//		data->data.Byte_3=Byte_3;
//		data->data.Byte_4=Byte_4;
//		data->data.Byte_5=Byte_5;
//		data->data.Byte_6=Byte_6;
//		data->data.Byte_7=DAIKIN_CRC_Calc(data->TxBuffDATA,7);
//		Start_send(data->TxBuffDATA);
//}

//void SearchAC_Step3(uint8_t Byte_4,uint8_t Byte_5,uint8_t Byte_6){
//			TxbuffLen=8;
//			SearchAC_Start3_data.data.Byte_0=0x80;
//			SearchAC_Start3_data.data.Byte_1=0x80;
//			SearchAC_Start3_data.data.Byte_2=0x04;
//			SearchAC_Start3_data.data.Byte_3=0x00;
//			SearchAC_Start3_data.data.Byte_4=Byte_4;
//			SearchAC_Start3_data.data.Byte_5=Byte_5;
//			SearchAC_Start3_data.data.Byte_6=Byte_6;
//			SearchAC_Start3_data.data.Byte_7=DAIKIN_CRC_Calc(SearchAC_Start3_data.TxBuffDATA,7);
//			Start_send(SearchAC_Start3_data.TxBuffDATA);
//	
//}

//void SearchAC_Step4(TxByte_4* data,uint8_t Byte_1){
//		TxbuffLen=4;
//		data->data.Byte_0=0x00;
//		data->data.Byte_1=Byte_1;
//		data->data.Byte_2=0x05;
//		data->data.Byte_3=DAIKIN_CRC_Calc(data->TxBuffDATA,3);
//		Start_send(data->TxBuffDATA);
//}

////00 00 20 04 XX 类型的数据
//void SearchAC_Step5(TxByte_5* data,uint8_t Byte_1){
//		TxbuffLen=5;
//		data->data.Byte_0=0x00;
//		data->data.Byte_1=Byte_1;
//		data->data.Byte_2=0x20;
//		data->data.Byte_3=0x04;
//		data->data.Byte_4=DAIKIN_CRC_Calc(data->TxBuffDATA,4);
//		Start_send(data->TxBuffDATA);
//}

//void SearchAC_Step6(TxByte_4* data,uint8_t Byte_2){ //00 00 2X XX类型的数据
//		TxbuffLen=4;
//		data->data.Byte_0=0x00;
//		data->data.Byte_1=0x00;
//		data->data.Byte_2=Byte_2;
//		data->data.Byte_3=DAIKIN_CRC_Calc(data->TxBuffDATA,3);
//		Start_send(data->TxBuffDATA);

//}
//void SearchAC_Step24(TxByte_24* data){
//		TxbuffLen=24;
//		data->data.Byte_0=0x00;
//		data->data.Byte_1=0xF0;
//		data->data.Byte_2=0x30;
//		data->data.Byte_3=0x10;
//	//data->data.Byte_4=0x2E;
//	//data->data.Byte_5=0xDD;
//		data->data.Byte_6=0x00;
//	//data->data.Byte_7=
//	//data->data.Byte_8=
//	//data->data.Byte_9=
//	//data->data.Byte_10=
//		data->data.Byte_11=data->data.Byte_12;
//		data->data.Byte_12=data->data.Byte_13;
//		data->data.Byte_13=0x00;
//		data->data.Byte_14=data->data.Byte_15;
//		data->data.Byte_15=data->data.Byte_16;
//		data->data.Byte_16=data->data.Byte_17;
//		data->data.Byte_17=data->data.Byte_18;
//		data->data.Byte_18=data->data.Byte_19;
//		data->data.Byte_19=data->data.Byte_20;
//		data->data.Byte_20=data->data.Byte_21;
//		data->data.Byte_21=data->data.Byte_22;	
//		data->data.Byte_22=0x02;
//		data->data.Byte_23=DAIKIN_CRC_Calc(data->TxBuffDATA,23);
//		Start_send(data->TxBuffDATA);
//}


////00 00 10开头数据
//void sendHeader_10_Packet(){ //默认
//		TxbuffLen=24;
//		uint8_t byte16=0;
//		Byte24_data.data.Byte_0=0x00;
//		Byte24_data.data.Byte_1=0x00;
//		Byte24_data.data.Byte_2=0x10;
//		Byte24_data.data.power =0x00;
//		Byte24_data.data.modeH =0x60;
//		Byte24_data.data.modeL =0x00;
//		Byte24_data.data.temp	 =Rx21_data.data.Byte_11;
//		Byte24_data.data.Byte_7=Rx21_data.data.Byte_12;
//		Byte24_data.data.fan	 =Rx21_data.data.Byte_13;
//		Byte24_data.data.Byte_9=Rx21_data.data.Byte_14;
//		Byte24_data.data.FanDirection=0x02;
//		Byte24_data.data.Byte_11=0x00;
//		Byte24_data.data.Byte_12=0x00;
//		Byte24_data.data.Byte_13=0x00;
//		Byte24_data.data.Byte_14=0x00;
//		Byte24_data.data.Byte_15=0x00;
//		if(Byte24_data.data.modeH==0x61&&Byte24_data.data.modeL==0x01){ byte16=0x80;}
//		Byte24_data.data.Byte_16=byte16;
//		Byte24_data.data.Byte_17=0x40;
//		Byte24_data.data.Byte_18=0x01;
//		Byte24_data.data.Byte_19=0x00;
//		Byte24_data.data.Byte_20=0x00;
//		Byte24_data.data.Byte_21=0x00;
//		Byte24_data.data.Byte_22=0x00;
//		Byte24_data.data.Byte_23=DAIKIN_CRC_Calc(Byte24_data.TxBuffDATA,23);
//		Start_send(Byte24_data.TxBuffDATA);
//}
////00 00 13开头的数据
//void sendHeader_13_Packet(){
//		TxbuffLen=8;
//		SearchAC_Byte8_data.data.Byte_0=0x00;
//		SearchAC_Byte8_data.data.Byte_1=0x00;
//		SearchAC_Byte8_data.data.Byte_2=0x13;
//		SearchAC_Byte8_data.data.Byte_3=0x00;
//		SearchAC_Byte8_data.data.Byte_4=0x60;
//		SearchAC_Byte8_data.data.Byte_5=0x00;
//		SearchAC_Byte8_data.data.Byte_6=0x00;
//		SearchAC_Byte8_data.data.Byte_7=DAIKIN_CRC_Calc(SearchAC_Byte8_data.TxBuffDATA,7);
//		Start_send(SearchAC_Byte8_data.TxBuffDATA);
//}
////80 00 10开头的数据
//void sendHeader_80_00_10_Packet(){
//		TxbuffLen=24;
//		Byte24_data.data.Byte_0=0x80;
//		Byte24_data.data.Byte_1=0x00;
//		Byte24_data.data.Byte_2=0x10;
//		Byte24_data.data.power =0x00;
//		Byte24_data.data.modeH =0x60;
//		Byte24_data.data.modeL =0x00;
//		Byte24_data.data.temp	 =Rx21_data.data.Byte_11;
//		Byte24_data.data.Byte_7=Rx21_data.data.Byte_12;
//		Byte24_data.data.fan	 =Rx21_data.data.Byte_13;
//		Byte24_data.data.Byte_9=Rx21_data.data.Byte_14;
//		Byte24_data.data.FanDirection=0x02;
//		Byte24_data.data.Byte_11=0x00;
//		Byte24_data.data.Byte_12=0x00;
//		Byte24_data.data.Byte_13=0x00;
//		Byte24_data.data.Byte_14=0x00;
//		Byte24_data.data.Byte_15=0x00;
//		Byte24_data.data.Byte_16=0x80;
//		Byte24_data.data.Byte_17=0x00;
//		Byte24_data.data.Byte_18=0x00;
//		Byte24_data.data.Byte_19=0x00;
//		Byte24_data.data.Byte_20=0x00;
//		Byte24_data.data.Byte_21=0x00;
//		Byte24_data.data.Byte_22=0x00;
//		Byte24_data.data.Byte_23=DAIKIN_CRC_Calc(Byte24_data.TxBuffDATA,23);
//		Start_send(Byte24_data.TxBuffDATA);
//	
//}
////00 00 12开头的数据
//void sendHeader_12_Packet(){
//	TxbuffLen=20;
//	Byte20_data.data.Byte_0=0x00;
//	Byte20_data.data.Byte_1=0x00;
//	Byte20_data.data.Byte_2=0x12;
//	Byte20_data.data.Byte_3=0x00;
//	Byte20_data.data.Byte_4=0x00;
//	Byte20_data.data.Byte_5=0x00;
//	Byte20_data.data.Byte_6=0x00;
//	Byte20_data.data.Byte_7=0x00;
//	Byte20_data.data.Byte_8=0x00;
//	Byte20_data.data.Byte_9=0x00;
//	Byte20_data.data.Byte_10=0x00;
//	Byte20_data.data.Byte_11=0xFF;
//	Byte20_data.data.Byte_12=0x00;
//	Byte20_data.data.Byte_13=0x00;
//	Byte20_data.data.Byte_14=0x00;
//	Byte20_data.data.Byte_15=0x00;
//	Byte20_data.data.Byte_16=0x00;
//	Byte20_data.data.Byte_17=0x00;
//	Byte20_data.data.Byte_18=0x00;
//	Byte20_data.data.Byte_19=DAIKIN_CRC_Calc(Byte20_data.TxBuffDATA,19);
//	Start_send(Byte20_data.TxBuffDATA);
//	
//}
////00 00 1F开头的数据
//void sendHeader_1F_Packet(){
//	TxbuffLen=3;
//	SearchAC_Byte4_data.data.Byte_0=0x00;
//	SearchAC_Byte4_data.data.Byte_1=0x00;
//	SearchAC_Byte4_data.data.Byte_2=0x1F;
//	SearchAC_Byte4_data.data.Byte_3=DAIKIN_CRC_Calc(SearchAC_Byte4_data.TxBuffDATA,3);
//	Start_send(SearchAC_Byte4_data.TxBuffDATA);
//	
//}
////80 00 18开头的数据
//void sendHeader_80_00_18_Packet(){
//	TxbuffLen=12;
//	Byte12_data.data.Byte_0=0x80;
//	Byte12_data.data.Byte_1=0x00;
//	Byte12_data.data.Byte_2=0x18;
//	Byte12_data.data.Byte_3=0x00;
//	Byte12_data.data.Byte_4=0x00;
//	Byte12_data.data.Byte_5=0x00;
//	Byte12_data.data.Byte_6=0x00;
//	Byte12_data.data.Byte_7=0x00;
//	Byte12_data.data.Byte_8=0x00;
//	Byte12_data.data.Byte_9=0x00;
//	Byte12_data.data.Byte_10=0x00;
//	Byte12_data.data.Byte_11=DAIKIN_CRC_Calc(Byte12_data.TxBuffDATA,11);
//	Start_send(Byte12_data.TxBuffDATA);
//	
//}
////00 00 11开头的数据
//void sendHeader_11_Packet(){
//	TxbuffLen=14;
//	Byte14_data.data.Byte_0=0x00;
//	Byte14_data.data.Byte_1=0x00;
//	Byte14_data.data.Byte_2=0x11;
//	Byte14_data.data.Byte_3=0x00;
//	Byte14_data.data.Byte_4=0x00;
//	Byte14_data.data.Byte_5=0x00;
//	Byte14_data.data.Byte_6=0x00;
//	Byte14_data.data.Byte_7=0x00;
//	Byte14_data.data.Byte_8=0x19;
//	Byte14_data.data.Byte_9=0x58;
//	Byte14_data.data.Byte_10=0x00;
//	Byte14_data.data.Byte_11=0x00;
//	Byte14_data.data.Byte_12=0x00;
//	Byte14_data.data.Byte_13=DAIKIN_CRC_Calc(Byte14_data.TxBuffDATA,13);
//	Start_send(Byte14_data.TxBuffDATA);
//	
//}

//uint8_t Start_Search(uint8_t flag){ 
//		static uint8_t Curren_Step=1;					//当前执行到的步数
//		static uint8_t i=0;										//发送控制次数计数
//		static uint8_t Byte4=0xFF;						
//		static uint8_t Byte5=0xFF;
//		static uint8_t Byte6=0xFF;
//		static uint8_t bit_mask_byte4 = 0x01;
//		static uint8_t bit_mask_byte5 = 0x01;
//		static uint8_t bit_mask_byte6 = 0x01;	
//		uint8_t ReturnState=0;
//		
//		switch(Curren_Step){
//			case 1:
//				SearchAC_Step1(&SearchAC_Byte4_data,0x01);
//				Curren_Step=2;
//				break;
//			case 2:
//				SearchAC_Step1(&SearchAC_Byte4_data,0x02);
//				Curren_Step=3;
//				break;
//			case 3:
//				SearchAC_Step1(&SearchAC_Byte4_data,0x02);
//				Curren_Step=4;
//				break;
//			case 4:
//				if(flag == 2) { 		// 未收到应答，定时发送
//						SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//						Curren_Step=5;
//				}else if(flag == 1) {
//					SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//				}
//				break;
//			case 5:
//						if(i == 24) {
//								SearchAC_Step3(Byte4,Byte5,Byte6);
//								Curren_Step = 6;	//发完一组切换到第六步
//								// 重置状态
//								i = 0;
//								bit_mask_byte5 = 0x01;
//								bit_mask_byte6 = 0x01;
//							
//						} else {
//							
//							if(flag == 2) { 		// 未收到应答，定时发送
//								if(Byte4 == 0xFF) {
//										Byte4 = 0xFB;	 				 // 初始状态转换
//										bit_mask_byte5 = 0x01; // 重置掩码
//										bit_mask_byte6 = 0x01; 
//								}else	if(Byte4 == 0xFB){
//										if(Byte5 == 0xF8) {
//												Byte5 = 0x78; // 特殊状态转换
//												bit_mask_byte6 = 0x01; // 准备处理Byte6
//										} else if(Byte5 != 0x78) {
//												// 逐步清零Byte5
//												Byte5 &= ~bit_mask_byte5;
//												bit_mask_byte5 <<= 1;
//												// 重置掩码如果完成一轮
//												if(bit_mask_byte5 == 0) bit_mask_byte5 = 0x01;
//										}else if(Byte5 == 0x78 && Byte6 != 0) {  // 处理Byte6（当Byte5就绪后）
//												Byte6 &= ~bit_mask_byte6;
//												bit_mask_byte6 <<= 1;
//												if(bit_mask_byte6 == 0) bit_mask_byte6 = 0x01;
//										}
//								}
//								SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//							}else if(flag==1){
//								
//								SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//								//pin1=InversionPin(pin1);	
//							}
//				
//					// 循环计数器递增（使用统一的i）
//						i++;
//				}
//				break;
//			case 6:
//				if(flag==2){					//如果没有收到回复就一直发
//					SearchAC_Step3(Byte4,Byte5,Byte6);
//				}else if(flag==1){		//如果收到应答判断一下
//						if(TxdataBuff[0]==0x40&&TxdataBuff[1]==0x00&&TxdataBuff[2]==0x04&&TxdataBuff[3]==0x1B){ //成功收到应答，0x40 0x00 0x04 0x1B
//							Curren_Step=7;	//切换到第七步
//							Byte4 = 0xFF;
//							Byte5 = 0xFF;
//							Byte6 = 0xFF;
//							i=0;
//						}else{						//没有正确接收到应答
//							Byte4 = 0xFF;
//							Byte5 = 0xFF;
//							Byte6 = 0xFF;
//							Curren_Step=4;	//切换到第四步，重新开始
//							i=0;
//						}
//				}
//				break;
//			case 7:
//				
//				if(i==24){
//					i=0;
//					bit_mask_byte4 = 0x01;
//					bit_mask_byte5 = 0x01;
//					bit_mask_byte6 = 0x01;
//					Curren_Step = 8;		
//				}else{
//		
//					if(flag==2){					//未收到应答，定时发送
//						if(Byte4!=0xFF&&Byte4!=0x00){
//							Byte4 &= ~bit_mask_byte4;
//							bit_mask_byte4 <<= 1;
//							if(bit_mask_byte4 == 0) bit_mask_byte4 = 0x01;
//						}else if(Byte4==0x00&&Byte5!=0xFF&&Byte5!=0x00){
//							Byte5 &= ~bit_mask_byte5;
//							bit_mask_byte5 <<= 1;
//							if(bit_mask_byte5 == 0) bit_mask_byte5 = 0x01;
//						}else if(Byte5==0x00&&Byte6!=0x00&Byte6!=0xFF){
//							Byte6 &= ~bit_mask_byte6;
//							bit_mask_byte6 <<= 1;
//							if(bit_mask_byte6 == 0) bit_mask_byte6 = 0x01;
//						}
//						SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//					}else if(flag==1){		//收到应答，发送
//																//处理应答
//						SearchAC_Step2(&SearchAC_Byte8_data, i, Byte4, Byte5, Byte6);
//						
//					}
//					
//					i++;
//				}
//				break;
//			case 8:
//				SearchAC_Step4(&SearchAC_Byte4_data,0x00);
//				Curren_Step = 9;		
//			break;
//			case 9:
//				if(i==17){
//					i=0;	
//					Curren_Step = 10;
//			
//				}else{
//						if(flag==1&&i==0){ //第一次发送00 00 05 F6，如果空调应答了，则发送下一条
//									if(TxdataBuff[0]==0x40&&TxdataBuff[1]==0x00&&TxdataBuff[2]==0x05&&TxdataBuff[3]==0xFB&&TxdataBuff[4]==0x78&&TxdataBuff[5]==0xC0){//空调应答正确则开始下面的步骤
//										i++;
//										SearchAC_Step4(&SearchAC_Byte4_data,i);
//									}
//						}else	if(flag==2){
//								SearchAC_Step4(&SearchAC_Byte4_data,i);
//								
//						}
//					i++;
//				}
//				break;
//			case 10:
//				
//					SearchAC_Step5(&SearchAC_Byte5_data,0x00); 		//00 00 20 04 3C
//					Curren_Step = 11;
//				
//				break;
//			
//			case 11:
//				if(flag==2){
//						Curren_Step = 10;
//				}else if(flag==1){                      				//接收到对应00 00 20 04 3C这条消息的空调的应答
//						memcpy(Rx20_data.TxBuffDATA,TxdataBuff,24);
//						SearchAC_Step6(&SearchAC_Byte4_data,0x23);
//						
//						Curren_Step = 12;
//				}
//				break;
//			case 12:
//				if(flag==2){
//						Curren_Step = 11;
//				}else if(flag==1){                      				//接收到对应00 00 23 40这条消息的空调的应答
//						SearchAC_Step6(&SearchAC_Byte4_data,0x21);	
//						Curren_Step = 13;
//				}
//				break;
//			case 13:
//				if(flag==2){																		//没有接收到对应00 00 21 53这条消息的空调的应答
//					
//						SearchAC_Step6(&SearchAC_Byte4_data,0x21);
//					
//				}else if(flag==1){                      				//接收到对应00 00 21 53这条消息的空调的应答
//						memcpy(Rx21_data.TxBuffDATA,TxdataBuff,24);
//						SearchAC_Step6(&SearchAC_Byte4_data,0x22);	//发送 00 00 22 90
//						Curren_Step = 14;
//				}
//		
//				break;
//			case 14:
//				if(flag==2){																		//没有接收到对应00 00 22 90这条消息的空调的应答
//					
//						SearchAC_Step6(&SearchAC_Byte4_data,0x22);
//					
//				}else if(flag==1){                      				//接收到对应00 00 22 90这条消息的空调的应答
//				
//						SearchAC_Step6(&SearchAC_Byte4_data,0x23);	//发送 00 00 23 40
//						Curren_Step = 15;
//				}
//			
//				break;
//			case 15:
//				if(flag==2){																		//没有接收到对应00 00 23 40这条消息的空调的应答
//					
//						SearchAC_Step6(&SearchAC_Byte4_data,0x22);
//					
//				}else if(flag==1){                      				//接收到对应00 00 23 40这条消息的空调的应答
//				
//						SearchAC_Step6(&SearchAC_Byte4_data,0x21);	//发送 00 00 21 53
//						Curren_Step = 16;
//				}
//				
//				break;
//			case 16:
//				 if(flag==1){                      				//接收到对应00 00 21 53这条消息的空调的应答
//						
//						SearchAC_Step24(&Rx20_data);	//发送 00	F0	30	10	2E	DD	00	20	10	20	10	01	13	00	68	C8	3F	0D	00	00	00	00	02 数据
//						Curren_Step = 17;
//				}
//				
//				//SearchAC_Step24(&Rx_Byte24_data);
//				break;
//			case 17:
//					//结束
//					ReturnState=SearchSuccess;
//					Curren_Step=1;
//				break;
//			default:
//					ReturnState=SearchContinue;
//				break;
//	}
//		return ReturnState;
//}


////第一组 00 00 10 开头 00 00 13开头的数据 
//uint8_t send_group1(uint8_t flag,uint8_t ConnectStep,uint8_t*GroupCount){
//	static uint8_t group1step=1;
//	switch(group1step){
//		case 1:
//			sendHeader_10_Packet();		//发送00 00 10开头的数据
//			group1step=2;
//			break;
//		case 2:
//			if(flag==1){						 //空调应答，再发送
//				sendHeader_13_Packet();//发送 00 00 13开头的数据
//				group1step=3;
//			}else if(flag==2){			//空调未答，不处理
//					
//			}
//			break;
//		case 3:
//			if(flag==1){						
//				if(ConnectStep==1){ //只在第一步的时候发送下面的数据
//						sendHeader_80_00_10_Packet();//发送80 00 10开头的数据
//						group1step=4;
//				}else{
//						sendHeader_12_Packet();			 //发送 00 00 12 开头的数据
//						group1step=5;
//				}
//			}else if(flag==2){	//空调未应答，不处理
//					
//			}
//			break;
//		case 4:
//			sendHeader_12_Packet();							//发送 00 00 12 开头的数据
//			group1step=5;
//			break;
//		case 5:				//发送完一组
//			send_GroupEnd(GroupCount);
//			group1step=1;
//			return ConnectStep+1;
//	}
//	
//	return ConnectStep;

//}
////第二组 00 00 10 和 00 00 13
//uint8_t send_group2(uint8_t flag,uint8_t ConnectStep,uint8_t*GroupCount){
//	static uint8_t group2step=1;
//	switch(group2step){
//		case 1:
//			if(flag==2){
//				sendHeader_10_Packet();		//发送00 00 10开头的数据
//				group2step=2;
//			}
//			break;
//		case 2:
//			if(flag==1){
//				sendHeader_13_Packet();		//发送00 00 13开头的数据		
//				group2step=3;
//			}
//			break;
//		case 3:	//发送完一组
//			send_GroupEnd(GroupCount);
//			group2step=1;
//			return ConnectStep+1;
//	}	
//	return ConnectStep;
//}
////第三组，10 13 1F 10 13 800018
//uint8_t send_group3(uint8_t flag,uint8_t ConnectStep){
//		static uint8_t group3step=1;
//		switch(group3step){
//			case 1:
//				if(flag==2){
//					sendHeader_10_Packet();		//发送00 00 10开头的数据
//					group3step=2;
//				}
//				break;
//			case 2:
//				if(flag==1){
//					sendHeader_13_Packet();		//发送00 00 13开头的数据
//					group3step=3;
//				}
//				break;
//			case 3:
//				if(flag==1){
//					sendHeader_1F_Packet();		//发送00 00 1F开头的数据
//					group3step=4;
//				}
//				break;
//			case 4:
//				if(flag==1){
//					sendHeader_10_Packet();		//发送00 00 13开头的数据
//					group3step=5;
//				}
//				break;
//			case 5:
//				if(flag==1){
//					sendHeader_13_Packet();		//发送00 00 13开头的数据
//					group3step=6;
//				}
//				break;
//			case 6:
//				if(flag==1){
//					sendHeader_80_00_18_Packet();		//发送80 00 18开头的数据
//					group3step=7;
//				}
//				break;
//			case 7:
//				if(flag==2){
//					//发送完一组
//				}
//				group3step=1;
//				return ConnectStep+1;
//				//break;
//		}

//	return ConnectStep;
//}

////发送第一种开始一个循环的开头的数据
//uint8_t send_StartCycleHeader1(uint8_t flag,uint8_t ConnectStep){
//	if(flag==2){
//		sendHeader_11_Packet();	//发送00 00 11开头的数据
//	}

//	return ConnectStep+1;
//}
////发送第二种开始一个循环的开头的数据
//uint8_t send_StartCycleHeader2(uint8_t flag,uint8_t ConnectStep){
//	if(flag==2){
//		SearchAC_Step24(&Rx20_data);//发送00 F0 30 开头的数据
//	}
//	return ConnectStep+1;
//	
//}
////一组数据的结尾
//void send_GroupEnd(uint8_t* GroupCount){
//	(*GroupCount)++;
//	SearchAC_Step5(&SearchAC_Byte5_data,*GroupCount);
//}

///*flag==2,未接收到数据计时110ms发送，flag==1，接收到空调的应答非堵塞延时25ms再发送
// *	保持和空调的连接
//**/
//void KeepConnect(uint8_t flag){
//		//F0								
//		//10 13 80 10 			00002004 这是一组数据
//		//12 								00012004
//		//10 13 1F 10 13 80	不发这个
//	
//			
//		//11								00022004
//		//下面这三组是一个周期
//		//10 13 12					00032004
//		//10 13 
//		//10 13 1F 10 13 80 
//		//F0
//		static uint8_t ConnectStep=1;
//		static uint8_t GroupCount=0;
//		switch(ConnectStep){
//			case 1:
//				ConnectStep=send_group1(flag,ConnectStep,&GroupCount); // 10 13 8010/10 13 12
//				break;
//			case 2:
//				ConnectStep=send_group2(flag,ConnectStep,&GroupCount); //10 13 
//				break;
//			case 3:
//				ConnectStep=send_group3(flag,ConnectStep);						//10 13 1F 10 13 80
//				break;
//			case 4:
//				ConnectStep=send_StartCycleHeader1(flag,ConnectStep); //00 00 11
//				break;
//			case 5:
//				ConnectStep=send_group1(flag,ConnectStep,&GroupCount);//10 13 12
//				break;
//			case 6:
//				ConnectStep=send_group2(flag,ConnectStep,&GroupCount);//10 13 
//				break;
//			case 7:
//				ConnectStep=send_group3(flag,ConnectStep);						//10 13 1F 10 13 80
//				break;
//			case 8:
//				ConnectStep=send_StartCycleHeader1(flag,ConnectStep); //00 F0 30
//				break;
//			case 9:
//				ConnectStep=send_group1(flag,ConnectStep,&GroupCount);//10 13 12
//				break;
//			case 10:
//				ConnectStep=send_group2(flag,ConnectStep,&GroupCount);//10 13 
//				break;
//			case 11:
//				ConnectStep=send_group3(flag,ConnectStep);						//10 13 1F 10 13 80
//				break;
//			case 12:
//				ConnectStep=4;
//				break;
//			
//		}


//}

////开启定时器TMR1
//void Open_TMR1(void){

//	TMR1_TimerInit(1666);//定时52us周期
//	TMR1_ITCfg(ENABLE, TMR0_3_IT_CYC_END);//开启定时中断
//	NVIC_SetPriority(TMR1_IRQn ,1);
//	NVIC_EnableIRQ( TMR1_IRQn);
//	//TMR1_Disable();
//}
////计算校验位，偶校验
//uint8_t calculate_parity(uint8_t data) {
//    uint8_t count = 0;
//    for (int i = 0; i < 8; i++) {
//        if (data & (1 << i)) {
//					count++;
//			}
//    }
//    return (count % 2) == 0; //如果1的个数是偶数返回0，如果是奇数返回1；
//	// 偶校验等价于
//	/*
//	if ((count % 2) == 0) {
//    return true;   // 1的个数是偶数
//	} else {
//    return false;  // 1的个数是奇数
//	}*/
//}
////发送
//void Start_send(uint8_t* pTxBuff)	{							//只要调用这个函数就会发送信号
//	
//	if(Sendstate==IDIE){
//			//TMR1_Disable();										
//			//TxIndex=0;															//数组索引置零
//			TxBuff=pTxBuff;
//			Sendstate =	START_BIT_LOW;							//初始化状态	
//			 //GPIOA_SetBits(GPIO_Pin_2);
//			Open_TMR1();	
//																							//打开定时器
//			//GPIOB_ResetBits(GPIO_Pin_7); 					//拉低pa7，起始位开始		
//		}
//		
//}

//void TMR1_IRQHandler(void)
//{	
//	
//	if( TMR1_GetITFlag( TMR0_3_IT_CYC_END ) ){
//			TMR1_ClearITFlag(TMR0_3_IT_CYC_END);
//		//pin1=InversionPin(pin1);
//		static  volatile uint8_t P=0;
//		static  volatile uint8_t BitCount=0;
//		static volatile uint8_t TxIndex=0;	
//				//R8_UART1_THR=Sendstate;
//			switch(Sendstate){
//				
///**/		case START_BIT_LOW:										// 1 AMI起始位低位，50us后切换到START_BIT_HIGH，AMI起始位高位
//					Sendstate=START_BIT_HIGH;
//					GPIOB_ResetBits(GPIO_Pin_7); 
//					//Sendstate=START_BIT_HIGH;
//					//R8_UART1_THR=Sendstate;
//					BitCount=0;
//					//pin1=InversionPin(pin1);
//					break;

//				case START_BIT_HIGH:									//2 AMI起始位高位，50us后进入START_BIT_HIGH，拉高电平
//					Sendstate=DATA_BIT_LOW;	
//					 //R8_UART1_THR=BitCount;
//					GPIOB_SetBits(GPIO_Pin_7); 
//					//pin1=InversionPin(pin1);																		//50us后切换DATA_BIT_LOW，AMI起始位结束
//					//R8_UART1_THR=Sendstate;
//					BitCount=0;
//					break;
//				case DATA_BIT_LOW:										//3 发送AMI数据位的低位
//						
//					if(BitCount<8){	
//						if((TxBuff[TxIndex]&(0x01<<BitCount))==1)						
//						{		
//							
//							GPIOB_SetBits(GPIO_Pin_7);		
//							//pin1=InversionPin(pin1);
//						}else if((TxBuff[TxIndex]&(0x01<<BitCount))==0){				
//																														
//							GPIOB_ResetBits(GPIO_Pin_7);
//							//pin1=InversionPin(pin1);			
//						}
//						Sendstate=DATA_BIT_HIGH;
//					}
//						//BitCount++;
//					break;
//				case DATA_BIT_HIGH:										//4 发送AMI数据位的高位
//																							/*逻辑0和逻辑1的高位都是50us高电平*/
//						BitCount++;	
//					if(BitCount<8){	
//						//	pin1=InversionPin(pin1);							
//							Sendstate=DATA_BIT_LOW;							//继续发送下一位，一共发送8位数据位
//							
//					}else if(BitCount==8){															//
//							BitCount=0;													//	必须在中断函数中马上修改，不能写在其它case中修改位计数清零		
//							P=calculate_parity(TxBuff[TxIndex]);
//							TxIndex++;
//						 Sendstate=PARITY_BIT_LOW;						//数据位发送完毕，切换到校验位
//							//R8_UART1_THR=Sendstate;
//							//pin1=InversionPin(pin1);
//					}	
//					GPIOB_SetBits(GPIO_Pin_7);
//					break;
//				case PARITY_BIT_LOW:									//5 AMI校验位
//						
//						//parity=calculate_parity(txdata[TxIndex]);	//偶校验方式计算校验位
//						Sendstate=PARITY_BIT_HIGH;
//						if(P==0){
//							//pin1=InversionPin(pin1);
//							 GPIOB_SetBits(GPIO_Pin_7);
//							
//												
//							//R8_UART1_THR=Sendstate;
//						}else if(P==1){
//							 //pin1=InversionPin(pin1);
//							GPIOB_ResetBits(GPIO_Pin_7);
//					
//							//R8_UART1_THR=Sendstate;
//						}
//				
//					break;
//				case PARITY_BIT_HIGH:									//6
//							//pin1=InversionPin(pin1);			//
//						GPIOB_SetBits(GPIO_Pin_7);				
//						Sendstate=STOP_BIT_LOW;	
//							//R8_UART1_THR=Sendstate;
//					break;
//				case STOP_BIT_LOW:										//7 停止位低位
//						//pin1=InversionPin(pin1);
//						GPIOB_SetBits(GPIO_Pin_7);	
//						Sendstate=STOP_BIT_HIGH;
//						//R8_UART1_THR=Sendstate;
//					break;
//				case STOP_BIT_HIGH:										//8 停止位高位
//						 //pin1=InversionPin(pin1);
//						GPIOB_SetBits(GPIO_Pin_7);
//												//TxbuffLen				/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
//																						//要立马在中断函数中修改TxIndex这个值
//															
//						if(TxIndex<TxbuffLen){									//TxbuffLen：发送数组的长度
//																	//TxIdex：TxBuff数组索引,指向数组下一个字节
//													
//									Sendstate=START_BIT_LOW;			//一个字节发送完成，准备发送下一字节
//									//R8_UART1_THR=Sendstate;
//																				
//						}else{												//TxIndex>=TxbuffLen,也就是发完了一包数据
//							TxIndex=0;		
//							Sendstate=IDIE;									//状态切换到空闲，表示发送完成
//							//R8_UART1_THR=Sendstate;
//							TMR1_Disable();							//发完一包数据，关闭定时器不再发送
//						}
//					break;
//			
//				
//				default:
//							TxIndex=0;
//						// pin1=InversionPin(pin1);
////						Sendstate=IDIE;
//					break;
//		
//}
//		//R8_UART1_THR=Sendstate;



//	
//	}


//}

////void TMR2_init(void)
////{
////	
////	TMR2_TimerInit( 16000 );//定时周期500us
////	TMR2_ITCfg(ENABLE, TMR0_3_IT_CYC_END);          
////  NVIC_EnableIRQ( TMR2_IRQn );
////	NVIC_SetPriority(TMR2_IRQn ,1);
////}



