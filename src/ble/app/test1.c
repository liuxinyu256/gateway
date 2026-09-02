#include "CH57x_common.h"



void HbsSet(void){


	GPIOB_ModeCfg(GPIO_Pin_10,GPIO_ModeOut_PP_5mA);
	GPIOB_SetBits(GPIO_Pin_10);






}
