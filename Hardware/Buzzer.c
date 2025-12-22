#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/*引脚配置*/
#define BUZZER_PORT GPIOA
#define BUZZER_PIN GPIO_Pin_8

/*蜂鸣器初始化*/
void Buzzer_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
    
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN); //低电平触发，初始高电平关闭
}

/*蜂鸣器控制*/
void Buzzer_Control(uint8_t status)
{
    if (status)
    {
        GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN); //低电平触发，设置低电平打开
    }
    else
    {
        GPIO_SetBits(BUZZER_PORT, BUZZER_PIN); //设置高电平关闭
    }
}

/*蜂鸣器鸣叫一次*/
void Buzzer_Beep(uint16_t duration)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN); //低电平触发，设置低电平打开
    Delay_ms(duration);
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN); //设置高电平关闭
}