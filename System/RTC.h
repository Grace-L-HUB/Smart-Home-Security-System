#ifndef __RTC_H
#define __RTC_H

#include "stm32f10x.h"

/**
  * @brief  RTC时间结构体
  */
typedef struct {
    uint8_t year;   /* 年份（0-99，代表2000-2099年） */
    uint8_t month;  /* 月份（1-12） */
    uint8_t day;    /* 日期（1-31） */
    uint8_t hour;   /* 小时（0-23） */
    uint8_t minute; /* 分钟（0-59） */
    uint8_t second; /* 秒钟（0-59） */
} RTC_TimeTypeDef;

/**
  * @brief  RTC初始化函数
  * @param  None
  * @retval None
  */
void RTC_Init(void);

/**
  * @brief  设置RTC时间
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_SetTime(RTC_TimeTypeDef* time);

/**
  * @brief  获取RTC时间
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_GetTime(RTC_TimeTypeDef* time);

/**
  * @brief  将RTC时间转换为秒数（从2000年1月1日开始）
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval 转换后的秒数
  */
uint32_t RTC_ConvertToSeconds(RTC_TimeTypeDef* time);

/**
  * @brief  将秒数转换为RTC时间（从2000年1月1日开始）
  * @param  seconds: 秒数
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_ConvertFromSeconds(uint32_t seconds, RTC_TimeTypeDef* time);

#endif /* __RTC_H */
