#include "RTC.h"

/**
  * @brief  检查年份是否为闰年
  * @param  year: 年份（0-99，代表2000-2099年）
  * @retval 1: 闰年，0: 平年
  */
static uint8_t RTC_IsLeapYear(uint8_t year) {
    year += 2000; // 转换为实际年份
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;
    }
    return 0;
}

/**
  * @brief  获取月份的天数
  * @param  year: 年份（0-99，代表2000-2099年）
  * @param  month: 月份（1-12）
  * @retval 该月份的天数
  */
static uint8_t RTC_GetDaysInMonth(uint8_t year, uint8_t month) {
    const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2 && RTC_IsLeapYear(year)) {
        return 29;
    }
    return days_in_month[month];
}

/**
  * @brief  RTC初始化函数
  * @param  None
  * @retval None
  */
void RTC_Init(void) {
    /* 使能PWR和BKP时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    /* 允许访问BKP寄存器 */
    PWR_BackupAccessCmd(ENABLE);
    
    /* 关闭LSE振荡器 */
    RCC_LSEConfig(RCC_LSE_OFF);
    
    /* 等待LSE关闭 */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET);
    
    /* 使能LSE振荡器 */
    RCC_LSEConfig(RCC_LSE_ON);
    
    /* 等待LSE稳定 */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
    
    /* 选择LSE作为RTC时钟源 */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    
    /* 使能RTC时钟 */
    RCC_RTCCLKCmd(ENABLE);
    
    /* 等待RTC寄存器同步 */
    RTC_WaitForSynchro();
    
    /* 等待RTC寄存器更新 */
    RTC_WaitForLastTask();
    
    /* 设置RTC预分频器 */
    RTC_SetPrescaler(32767); /* LSE频率为32.768kHz，预分频后得到1Hz */
    
    /* 等待RTC寄存器更新 */
    RTC_WaitForLastTask();
    
    /* 检查RTC是否已经初始化过 */
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
        /* 第一次初始化，设置当前时间 */
        RTC_TimeTypeDef init_time;
        init_time.year = 25;
        init_time.month = 12;
        init_time.day = 13;
        init_time.hour = 21;
        init_time.minute = 51;
        init_time.second = 0;
        
        RTC_SetTime(&init_time);
        
        /* 写入备份寄存器，标记RTC已初始化 */
        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
}

/**
  * @brief  设置RTC时间
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_SetTime(RTC_TimeTypeDef* time) {
    uint32_t seconds;
    
    /* 转换时间为秒数 */
    seconds = RTC_ConvertToSeconds(time);
    
    /* 设置RTC计数器值 */
    RTC_SetCounter(seconds);
    
    /* 等待RTC寄存器更新 */
    RTC_WaitForLastTask();
}

/**
  * @brief  获取RTC时间
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_GetTime(RTC_TimeTypeDef* time) {
    uint32_t seconds;
    
    /* 获取RTC计数器值 */
    seconds = RTC_GetCounter();
    
    /* 转换秒数为时间 */
    RTC_ConvertFromSeconds(seconds, time);
}

/**
  * @brief  将RTC时间转换为秒数（从2000年1月1日开始）
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval 转换后的秒数
  */
uint32_t RTC_ConvertToSeconds(RTC_TimeTypeDef* time) {
    uint32_t seconds = 0;
    uint8_t i;
    
    /* 计算年份的秒数 */
    for (i = 0; i < time->year; i++) {
        if (RTC_IsLeapYear(i)) {
            seconds += 31622400; /* 闰年的秒数：366 * 24 * 60 * 60 */
        } else {
            seconds += 31536000; /* 平年的秒数：365 * 24 * 60 * 60 */
        }
    }
    
    /* 计算月份的秒数 */
    for (i = 1; i < time->month; i++) {
        seconds += RTC_GetDaysInMonth(time->year, i) * 86400; /* 一个月的秒数 */
    }
    
    /* 计算日期的秒数 */
    seconds += (time->day - 1) * 86400;
    
    /* 计算小时的秒数 */
    seconds += time->hour * 3600;
    
    /* 计算分钟的秒数 */
    seconds += time->minute * 60;
    
    /* 加上秒数 */
    seconds += time->second;
    
    return seconds;
}

/**
  * @brief  将秒数转换为RTC时间（从2000年1月1日开始）
  * @param  seconds: 秒数
  * @param  time: 指向RTC_TimeTypeDef结构体的指针
  * @retval None
  */
void RTC_ConvertFromSeconds(uint32_t seconds, RTC_TimeTypeDef* time) {
    uint32_t remaining_seconds = seconds;
    uint8_t year = 0;
    uint8_t month = 1;
    
    /* 计算年份 */
    while (1) {
        uint32_t year_seconds;
        
        if (RTC_IsLeapYear(year)) {
            year_seconds = 31622400; /* 闰年的秒数：366 * 24 * 60 * 60 */
        } else {
            year_seconds = 31536000; /* 平年的秒数：365 * 24 * 60 * 60 */
        }
        
        if (remaining_seconds >= year_seconds) {
            remaining_seconds -= year_seconds;
            year++;
        } else {
            break;
        }
    }
    
    /* 计算月份 */
    while (1) {
        uint32_t month_seconds = RTC_GetDaysInMonth(year, month) * 86400;
        
        if (remaining_seconds >= month_seconds) {
            remaining_seconds -= month_seconds;
            month++;
        } else {
            break;
        }
    }
    
    /* 计算日期 */
    time->day = remaining_seconds / 86400 + 1;
    remaining_seconds %= 86400;
    
    /* 计算小时 */
    time->hour = remaining_seconds / 3600;
    remaining_seconds %= 3600;
    
    /* 计算分钟 */
    time->minute = remaining_seconds / 60;
    remaining_seconds %= 60;
    
    /* 计算秒数 */
    time->second = remaining_seconds;
    
    /* 设置年份和月份 */
    time->year = year;
    time->month = month;
}
