#include "stm32f10x.h"                  // Device header

/*引脚配置*/
#define IR_PORT GPIOA
#define IR_PIN GPIO_Pin_1

/*红外传感器初始化*/
void IR_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = IR_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IR_PORT, &GPIO_InitStructure);
}

/*获取红外传感器状态*/
uint8_t IR_GetStatus(void)
{
    //返回0表示检测到物体，返回1表示未检测到物体
    return GPIO_ReadInputDataBit(IR_PORT, IR_PIN);
}