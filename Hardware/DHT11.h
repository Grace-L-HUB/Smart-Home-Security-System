#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"                  // Device header
#include "delay.h"

//DHT11引脚宏定义
#define DHT11_GPIO_PORT  GPIOA
#define DHT11_GPIO_PIN   GPIO_Pin_0
#define DHT11_GPIO_CLK   RCC_APB2Periph_GPIOA

//输出状态定义
#define OUT 1
#define IN  0

//控制DHT11引脚输出高低电平
#define DHT11_Low  GPIO_ResetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)
#define DHT11_High GPIO_SetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)

//函数声明
uint8_t DHT11_Init(void);
uint8_t DHT11_ReadData(uint8_t *humi, uint8_t *temp); // 修正参数顺序，先湿度后温度
uint8_t DHT11_ReadByte(void);
uint8_t DHT11_ReadBit(void);
void DHT11_Mode(uint8_t mode);
uint8_t DHT11_Check(void);
void DHT11_Rst(void);

#endif