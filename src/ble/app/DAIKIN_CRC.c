#include "CH57x_common.h"

/*******************************************************************************
* Function Name  : DAIKIN_CRC_Calc
* Description    : CRC校验（计算方式实现） 大金CRC校验
* Input          : bufData - 需要校验的数据
* Input          : buflen - 需要校验的字节数
* Output         : None
* Return         : Crc校验值
*******************************************************************************/
uint8_t DAIKIN_CRC_Calc(uint8_t *bufData, uint16_t buflen)
{
    unsigned char init_value = 0x00;
    unsigned char xor_value = 0xd9;
    unsigned char i, j;
    
    for (i = 0; i < buflen; i++)
    {
        for (j = 0x01; j != 0; j <<= 1)
		{
        	if ((init_value & 0x01) != 0)
            {
                init_value >>= 1;
                init_value ^= xor_value;
            }
            else
            {
                init_value >>= 1;
            }
    		if ((bufData[i] & j) != 0)
			{
        		init_value ^= xor_value;
    		}
		}
    }
    return init_value; 
} 
