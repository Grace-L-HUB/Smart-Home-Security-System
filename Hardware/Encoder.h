#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

/**
  * 函    数：编码器初始化
  * 参    数：无
  * 返 回 值：无
  */
void Encoder_Init(void);

/**
  * 函    数：获取编码器计数值
  * 参    数：无
  * 返 回 值：编码器计数值，范围：-32768~32767
  */
int16_t Encoder_GetCount(void);

/**
  * 函    数：获取编码器按键状态
  * 参    数：无
  * 返 回 值：按键状态，1表示按键按下，0表示未按下
  */
uint8_t Encoder_GetKeyStatus(void);

/**
  * 函    数：更新编码器计数值
  * 参    数：无
  * 返 回 值：无
  * 说    明：需要在主循环中定期调用此函数来更新编码器计数值
  */
void Encoder_Update(void);

#endif /* __ENCODER_H */
