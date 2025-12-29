#include "stm32f10x.h"                  // Device header
#include <string.h>
#include <stdlib.h>
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "DHT11.h"
#include "IR.h"
#include "Buzzer.h"
#include "Encoder.h"
#include "W25Q64.h"
#include "RTC.h"

//系统模式枚举
typedef enum {
    MODE_ARMED = 0,      //布防模式
    MODE_HOME,           //居家模式
    MODE_DEBUG           //调试模式
} SystemMode_t;

//系统状态结构体
typedef struct {
    SystemMode_t mode;           //系统模式
    uint8_t temperature;         //温度值
    uint8_t humidity;            //湿度值
    uint8_t ir_status;           //红外传感器状态
    uint8_t alarm_status;        //报警状态
    uint16_t alarm_count;        //报警计数，用于超时关闭
    uint8_t temp_threshold_low;  //温度下限阈值
    uint8_t temp_threshold_high; //温度上限阈值
    uint8_t humi_threshold_low;  //湿度下限阈值
    uint8_t humi_threshold_high; //湿度上限阈值
} SystemStatus_t;

//全局变量
SystemStatus_t system_status;
char serial_command_buffer[64];  // 串口命令缓冲区
uint8_t serial_command_length = 0;  // 命令长度
uint8_t serial_command_received = 0;  // 命令接收完成标志
uint32_t record_index = 0;  // 当前记录索引

// 数据记录相关常量
#define MAX_RECORDS           10000                   // 最大记录数（W25Q64容量大，可存储更多记录）

//函数声明
void System_Init(void);
void System_Update(void);
void System_Display(void);
void System_SerialSend(void);
void System_HandleAlarm(void);
void System_SwitchMode(SystemMode_t new_mode);
void System_HandleSerialCommand(void);
void System_ParseCommand(char *command);

int main(void)
{
    /*系统初始化*/
    System_Init();
    
    /*显示欢迎界面*/
    OLED_Clear();
    OLED_ShowString(1, 1, "Smart Home Security");
    OLED_ShowString(2, 1, "System Starting...");
    Delay_ms(1000);
    
    /*串口发送启动信息*/
    Serial_Printf("[INFO] System Starting...\n");
    
    while (1)
    {
        /*更新系统状态*/
        System_Update();
        
        /*处理报警逻辑*/
        System_HandleAlarm();
        
        /*刷新OLED显示*/
        System_Display();
        
        /*发送串口数据*/
        System_SerialSend();
        
        /*延时，控制循环频率*/
        Delay_ms(500);
    }
}

/**
  * 函    数：系统初始化
  * 参    数：无
  * 返 回 值：无
  */
void System_Init(void)
{
    /*硬件初始化*/
    OLED_Init();
    Serial_Init();
    DHT11_Init();
    IR_Init();
    Buzzer_Init();
    Encoder_Init();
    W25Q64_Init();
    RTC_Init();
    
    /*系统状态初始化*/
    system_status.mode = MODE_ARMED;
    system_status.temperature = 0;
    system_status.humidity = 0;
    system_status.ir_status = IR_GetStatus(); //初始化为实际红外传感器状态
    system_status.alarm_status = 0;
    system_status.alarm_count = 0;
    
    /*温湿度报警阈值初始化*/
    system_status.temp_threshold_low = 10;   //默认温度下限10°C
    system_status.temp_threshold_high = 30;  //默认温度上限30°C
    system_status.humi_threshold_low = 30;   //默认湿度下限30%
    system_status.humi_threshold_high = 80;  //默认湿度上限80%
    
    /*从W25Q64读取系统配置*/
    SystemConfig_t config;
    uint8_t config_result = W25Q64_ReadConfig(&config);
    if (config_result == 0)
    {
        /*配置有效，使用读取的值*/
        system_status.temp_threshold_low = config.temp_threshold_low;
        system_status.temp_threshold_high = config.temp_threshold_high;
        system_status.humi_threshold_low = config.humi_threshold_low;
        system_status.humi_threshold_high = config.humi_threshold_high;
        Serial_Printf("[INFO] System configuration loaded from W25Q64\n");
        
        /*检查是否需要更新湿度阈值（从旧的40%下限更新为新的30%下限）*/
        if (system_status.humi_threshold_low == 40)
        {
            system_status.humi_threshold_low = 30;
            /*保存更新后的配置到W25Q64*/
            config.humi_threshold_low = system_status.humi_threshold_low;
            W25Q64_WriteConfig(&config);
            Serial_Printf("[INFO] Humidity threshold updated to new default: 30-80%%\n");
        }
    }
    else
    {
        /*配置无效，使用默认值并保存*/
        config.temp_threshold_low = system_status.temp_threshold_low;
        config.temp_threshold_high = system_status.temp_threshold_high;
        config.humi_threshold_low = system_status.humi_threshold_low;
        config.humi_threshold_high = system_status.humi_threshold_high;
        W25Q64_WriteConfig(&config);
        Serial_Printf("[INFO] Default system configuration saved to W25Q64\n");
    }
    
    /*从W25Q64读取记录索引*/
    record_index = W25Q64_ReadRecordIndex();
    /*强制清除所有历史记录，确保新记录使用正确格式*/
    record_index = 0;
    W25Q64_ClearAllRecords();
    W25Q64_WriteRecordIndex(record_index);
    Serial_Printf("[INFO] History cleared to ensure proper record format\n");
    Serial_Printf("[INFO] Record index initialized: %lu\n", record_index);
    
    /*确保蜂鸣器关闭*/
    Buzzer_Control(0);
    
    /*串口发送初始化完成信息*/
    Serial_Printf("[INFO] System Initialized\n");
    Serial_Printf("[MODE]ARMED\n");
}

/**
  * 函    数：更新系统状态
  * 参    数：无
  * 返 回 值：无
  */
void System_Update(void)
{
    /*更新编码器状态*/
    Encoder_Update();
    int16_t encoder_count = Encoder_GetCount();
    
    /*处理串口命令*/
    if (serial_command_received)
    {
        serial_command_received = 0;
        System_HandleSerialCommand();
    }
    
    /*处理编码器按键，按动一次切换一个模式*/
    if (Encoder_GetKeyStatus() == 1)
    {
        //按固定顺序切换模式：MODE_ARMED → MODE_HOME → MODE_DEBUG → MODE_ARMED
        SystemMode_t next_mode = (SystemMode_t)((system_status.mode + 1) % 3);
        System_SwitchMode(next_mode);
    }
    
    /*读取DHT11温湿度数据*/
    static uint8_t dht11_count = 0;
    if (++dht11_count >= 10) //每5秒读取一次温湿度，减少读取频率
    {
        dht11_count = 0;
        
        // 添加错误处理和重试机制
        uint8_t retry = 0;
        uint8_t result = 1;
        uint8_t temp_read = system_status.temperature;
        uint8_t humi_read = system_status.humidity;
        
        while (retry < 5) // 最多重试5次
        {
            // 传递当前系统模式给DHT11_ReadData，以便只在调试模式下打印调试信息
            result = DHT11_ReadData(&humi_read, &temp_read, system_status.mode);
            
            // 如果读取成功，立即更新并退出
            if (result == 0)
            {
                system_status.temperature = temp_read;
                system_status.humidity = humi_read;
                break;
            }
            
            retry++;
            Delay_ms(200); // 重试前等待200ms
        }
        
        // 即使所有重试都失败，也使用最后一次读取到的数据
        // 不再恢复为初始值，因为DHT11_ReadData已经确保了数据的有效性
        system_status.temperature = temp_read;
        system_status.humidity = humi_read;
        

    }
    
    /*温湿度阈值判断*/
    if (system_status.mode == MODE_ARMED) // 仅在布防模式下触发阈值报警
    {
        // 检查温度是否超出阈值
        if (system_status.temperature < system_status.temp_threshold_low || 
            system_status.temperature > system_status.temp_threshold_high)
        {
            system_status.alarm_status = 1;
            system_status.alarm_count = 0;
            Buzzer_Beep(500); // 蜂鸣器响500ms
            Serial_Printf("[ALARM] Temperature out of range! Current: %d°C (Threshold: %d-%d°C)\n", 
                         system_status.temperature, 
                         system_status.temp_threshold_low, 
                         system_status.temp_threshold_high);
        }
        
        // 检查湿度是否超出阈值
        if (system_status.humidity < system_status.humi_threshold_low || 
            system_status.humidity > system_status.humi_threshold_high)
        {
            system_status.alarm_status = 1;
            system_status.alarm_count = 0;
            Buzzer_Beep(500); // 蜂鸣器响500ms
            Serial_Printf("[ALARM] Humidity out of range! Current: %d%% (Threshold: %d-%d%%)\n", 
                         system_status.humidity, 
                         system_status.humi_threshold_low, 
                         system_status.humi_threshold_high);
        }
    }
    
    /*读取红外传感器状态*/
    system_status.ir_status = IR_GetStatus();
    
    /*数据记录功能已移除定时记录，改为在红外报警时记录*/
}

/**
  * 函    数：处理报警逻辑
  * 参    数：无
  * 返 回 值：无
  */
void System_HandleAlarm(void)
{
    /*确保蜂鸣器默认关闭*/
    static uint8_t last_alarm_status = 0;
    
    /*根据当前模式处理报警*/
    switch (system_status.mode)
    {
        case MODE_ARMED:
            //布防模式：红外触发立即报警
            if (system_status.ir_status == 0) //红外检测到物体
            {
                if (system_status.alarm_status == 0) //状态变化时触发
                {
                    system_status.alarm_status = 1;
                    system_status.alarm_count = 0;
                    Buzzer_Beep(500); //蜂鸣器响500ms
                    Serial_Printf("[ALARM]INTRUSION!\n");
                    
                    // 记录报警数据
                    uint32_t timestamp = RTC_GetCounter(); // 直接获取RTC计数器值作为时间戳
                    
                    DataRecord_t record;
                    record.timestamp = timestamp;
                    record.temperature = system_status.temperature;
                    record.humidity = system_status.humidity;
                    record.system_mode = system_status.mode;
                    record.ir_status = system_status.ir_status;
                    
                    W25Q64_WriteRecord(&record, record_index);
                    
                    if (++record_index >= MAX_RECORDS)
                    {
                        record_index = 0;
                    }
                    W25Q64_WriteRecordIndex(record_index);
                }
            }
            else
            {
                //红外恢复正常，重置报警状态
                if (system_status.alarm_status == 1)
                {
                    system_status.alarm_status = 0;
                    system_status.alarm_count = 0;
                    Buzzer_Control(0); //确保蜂鸣器关闭
                    Serial_Printf("[INFO]Alarm Stopped\n");
                }
            }
            break;
            
        case MODE_HOME:
            //居家模式：红外触发仅记录，不报警
            if (system_status.ir_status == 0) //红外检测到物体
            {
                //只在状态变化时记录
                if (system_status.alarm_status == 0)
                {
                    system_status.alarm_status = 1;
                    Buzzer_Control(0); //确保蜂鸣器关闭
                    Serial_Printf("[INFO]Motion Detected\n");
                    
                    // 记录红外检测数据
                    uint32_t timestamp = RTC_GetCounter(); // 直接获取RTC计数器值作为时间戳
                    
                    DataRecord_t record;
                    record.timestamp = timestamp;
                    record.temperature = system_status.temperature;
                    record.humidity = system_status.humidity;
                    record.system_mode = system_status.mode;
                    record.ir_status = system_status.ir_status;
                    
                    W25Q64_WriteRecord(&record, record_index);
                    
                    if (++record_index >= MAX_RECORDS)
                    {
                        record_index = 0;
                    }
                    W25Q64_WriteRecordIndex(record_index);
                }
            }
            else
            {
                system_status.alarm_status = 0;
            }
            break;
            
        case MODE_DEBUG:
            //调试模式：不报警，确保蜂鸣器关闭
            if (system_status.alarm_status == 1)
            {
                system_status.alarm_status = 0;
                system_status.alarm_count = 0;
                Buzzer_Control(0); //确保蜂鸣器关闭
            }
            break;
    }
    
    /*确保蜂鸣器在无报警时关闭*/
    if (system_status.alarm_status == 0 && last_alarm_status == 1)
    {
        Buzzer_Control(0); //确保蜂鸣器关闭
    }
    last_alarm_status = system_status.alarm_status;
}

/**
  * 函    数：切换系统模式
  * 参    数：new_mode 新的系统模式
  * 返 回 值：无
  */
void System_SwitchMode(SystemMode_t new_mode)
{
    if (system_status.mode != new_mode)
    {
        system_status.mode = new_mode;
        
        /*发送模式切换信息*/
        switch (new_mode)
        {
            case MODE_ARMED:
                Serial_Printf("[MODE]ARMED\n");
                break;
            case MODE_HOME:
                Serial_Printf("[MODE]HOME\n");
                break;
            case MODE_DEBUG:
                Serial_Printf("[MODE]DEBUG\n");
                break;
        }
        
        /*模式切换时关闭蜂鸣器*/
        Buzzer_Control(0);
        
        /*蜂鸣器提示*/
        Buzzer_Beep(100);
    }
}

/**
  * 函    数：刷新OLED显示
  * 参    数：无
  * 返 回 值：无
  */
void System_Display(void)
{
    static uint8_t last_mode = 0xFF;
    static uint8_t last_temperature = 0xFF;
    static uint8_t last_humidity = 0xFF;
    static uint8_t last_ir_status = 0xFF;
    static uint8_t last_alarm_status = 0xFF;
    static uint8_t display_counter = 0;
    
    /*降低OLED刷新频率，每2秒刷新一次*/
    if (++display_counter < 4) // 主循环周期为500ms，4个周期即2秒
    {
        return;
    }
    display_counter = 0;
    
    /*清屏*/
    OLED_Clear();
    
    /*显示系统模式*/
    OLED_ShowString(1, 1, "Mode:");
    switch (system_status.mode)
    {
        case MODE_ARMED:
            OLED_ShowString(1, 6, "ARMED  ");
            break;
        case MODE_HOME:
            OLED_ShowString(1, 6, "HOME   ");
            break;
        case MODE_DEBUG:
            OLED_ShowString(1, 6, "DEBUG  ");
            break;
    }
    
    /*显示温湿度*/
    OLED_ShowString(2, 1, "Temp:");
    OLED_ShowNum(2, 6, system_status.temperature, 2);
    OLED_ShowString(2, 8, "C");
    
    OLED_ShowString(2, 10, "Hum:");
    OLED_ShowNum(2, 14, system_status.humidity, 2);
    OLED_ShowString(2, 16, "%");
    
    /*显示红外传感器状态*/
    OLED_ShowString(3, 1, "IR:");
    if (system_status.ir_status == 0)
    {
        OLED_ShowString(3, 4, "DETECTED");
    }
    else
    {
        OLED_ShowString(3, 4, "CLEAR   ");
    }
    
    /*显示报警状态*/
    OLED_ShowString(4, 1, "Alarm:");
    if (system_status.alarm_status == 1)
    {
        OLED_ShowString(4, 7, "ON ");
    }
    else
    {
        OLED_ShowString(4, 7, "OFF");
    }
}

/**
  * 函    数：发送串口数据
  * 参    数：无
  * 返 回 值：无
  */
void System_SerialSend(void)
{
    /*降低发送频率，从500ms改为2秒发送一次数据*/
    static uint8_t send_counter = 0;
    if (++send_counter >= 4) // 每2秒发送一次
    {
        send_counter = 0;
        /*发送周期数据*/
        Serial_Printf("[DATA]Temp:%d,Humi:%d,IR:%d\n", 
                     system_status.temperature, 
                     system_status.humidity, 
                     system_status.ir_status);
    }
}

/**
  * 函    数：处理串口命令
  * 参    数：无
  * 返 回 值：无
  */
void System_HandleSerialCommand(void)
{
    Serial_Printf("[INFO] Received command: %s\n", serial_command_buffer);
    System_ParseCommand(serial_command_buffer);
}

/**
  * 函    数：解析并执行串口命令
  * 参    数：command 要解析的命令字符串
  * 返 回 值：无
  */
void System_ParseCommand(char *command)
{
    // 简单的命令解析，支持以下命令：
    // help - 显示帮助信息
    // mode <num> - 切换系统模式 (0:布防, 1:居家, 2:调试)
    // status - 显示系统状态
    // reset - 重置系统
    
    if (strncmp(command, "help", 4) == 0)
    {
        Serial_Printf("[HELP] Available commands:\n");
        Serial_Printf("[HELP] help - Show this help message\n");
        Serial_Printf("[HELP] mode <0-2> - Switch system mode (0:ARMED, 1:HOME, 2:DEBUG)\n");
        Serial_Printf("[HELP] status - Show system status\n");
        Serial_Printf("[HELP] reset - Reset the system\n");
        Serial_Printf("[HELP] threshold temp <low> <high> - Set temperature thresholds\n");
        Serial_Printf("[HELP] threshold humi <low> <high> - Set humidity thresholds\n");
        Serial_Printf("[HELP] history [count] - Show historical data records\n");
        Serial_Printf("[HELP] export - Export data records in CSV format\n");
        Serial_Printf("[HELP] clear_history - Clear all historical data\n");
        Serial_Printf("[HELP] time - Show current time\n");
        Serial_Printf("[HELP] time <YY> <MM> <DD> <HH> <mm> <SS> - Set current time\n");
    }
    else if (strncmp(command, "mode", 4) == 0)
    {
        int mode_int = atoi(command + 5);
        if (mode_int >= 0 && mode_int <= 2)
        {
            System_SwitchMode((SystemMode_t)mode_int);
            Serial_Printf("[INFO] Mode switched to %d\n", mode_int);
        }
        else
        {
            Serial_Printf("[ERROR] Invalid mode. Use 0-2\n");
        }
    }
    else if (strncmp(command, "status", 6) == 0)
    {
        Serial_Printf("[STATUS] Mode: %s\n", 
                     system_status.mode == MODE_ARMED ? "ARMED" : 
                     system_status.mode == MODE_HOME ? "HOME" : "DEBUG");
        Serial_Printf("[STATUS] Temperature: %d°C\n", system_status.temperature);
        Serial_Printf("[STATUS] Humidity: %d%%\n", system_status.humidity);
        Serial_Printf("[STATUS] IR Status: %s\n", 
                     system_status.ir_status == 0 ? "DETECTED" : "CLEAR");
        Serial_Printf("[STATUS] Alarm Status: %s\n", 
                     system_status.alarm_status == 0 ? "OFF" : "ON");
        Serial_Printf("[STATUS] Temp Threshold: %d-%d°C\n", 
                     system_status.temp_threshold_low, 
                     system_status.temp_threshold_high);
        Serial_Printf("[STATUS] Humi Threshold: %d-%d%%\n", 
                     system_status.humi_threshold_low, 
                     system_status.humi_threshold_high);
    }
    else if (strncmp(command, "reset", 5) == 0)
    {
        Serial_Printf("[INFO] System resetting...\n");
        Delay_ms(500);
        NVIC_SystemReset(); // 系统复位
    }
    else if (strncmp(command, "threshold", 9) == 0)
    {
        // 解析threshold命令
        // 格式：threshold temp <low> <high> 或 threshold humi <low> <high>
        if (strstr(command, "temp") != NULL)
        {
            // 设置温度阈值
            int low, high;
            if (sscanf(command, "threshold temp %d %d", &low, &high) == 2)
            {
                if (low >= 0 && low <= 100 && high >= 0 && high <= 100 && low < high)
                {
                    system_status.temp_threshold_low = (uint8_t)low;
                    system_status.temp_threshold_high = (uint8_t)high;
                    
                    /*保存配置到W25Q64*/
                    SystemConfig_t config;
                    config.temp_threshold_low = system_status.temp_threshold_low;
                    config.temp_threshold_high = system_status.temp_threshold_high;
                    config.humi_threshold_low = system_status.humi_threshold_low;
                    config.humi_threshold_high = system_status.humi_threshold_high;
                    W25Q64_WriteConfig(&config);
                    
                    Serial_Printf("[INFO] Temperature thresholds set to %d-%d°C\n", low, high);
                }
                else
                {
                    Serial_Printf("[ERROR] Invalid temperature thresholds. Use 0-100, low < high\n");
                }
            }
            else
            {
                Serial_Printf("[ERROR] Invalid format. Use: threshold temp <low> <high>\n");
            }
        }
        else if (strstr(command, "humi") != NULL)
        {
            // 设置湿度阈值
            int low, high;
            if (sscanf(command, "threshold humi %d %d", &low, &high) == 2)
            {
                if (low >= 0 && low <= 100 && high >= 0 && high <= 100 && low < high)
                {
                    system_status.humi_threshold_low = (uint8_t)low;
                    system_status.humi_threshold_high = (uint8_t)high;
                    
                    /*保存配置到W25Q64*/
                    SystemConfig_t config;
                    config.temp_threshold_low = system_status.temp_threshold_low;
                    config.temp_threshold_high = system_status.temp_threshold_high;
                    config.humi_threshold_low = system_status.humi_threshold_low;
                    config.humi_threshold_high = system_status.humi_threshold_high;
                    W25Q64_WriteConfig(&config);
                    
                    Serial_Printf("[INFO] Humidity thresholds set to %d-%d%%\n", low, high);
                }
                else
                {
                    Serial_Printf("[ERROR] Invalid humidity thresholds. Use 0-100, low < high\n");
                }
            }
            else
            {
                Serial_Printf("[ERROR] Invalid format. Use: threshold humi <low> <high>\n");
            }
        }
        else
        {
            Serial_Printf("[ERROR] Invalid threshold type. Use 'temp' or 'humi'\n");
        }
    }
    else if (strncmp(command, "time", 4) == 0)
    {
        // time命令：显示当前时间
        if (strlen(command) == 4 || command[4] == ' ' && command[5] == '\0')
        {
            RTC_TimeTypeDef current_time;
            RTC_GetTime(&current_time);
            Serial_Printf("[INFO] Current time: 20%02d-%02d-%02d %02d:%02d:%02d\n", 
                         current_time.year, current_time.month, current_time.day, 
                         current_time.hour, current_time.minute, current_time.second);
        }
        // time <YY> <MM> <DD> <HH> <mm> <SS>命令：设置当前时间
        else
        {
            uint8_t year, month, day, hour, minute, second;
            if (sscanf(command, "time %hhu %hhu %hhu %hhu %hhu %hhu", &year, &month, &day, &hour, &minute, &second) == 6)
            {
                // 验证时间参数的有效性
                if (month >= 1 && month <= 12 && day >= 1 && day <= 31 && 
                            hour <= 23 && minute <= 59 && second <= 59)
                {
                    RTC_TimeTypeDef set_time;
                    set_time.year = year;
                    set_time.month = month;
                    set_time.day = day;
                    set_time.hour = hour;
                    set_time.minute = minute;
                    set_time.second = second;
                    
                    RTC_SetTime(&set_time);
                    Serial_Printf("[INFO] Time set to: 20%02d-%02d-%02d %02d:%02d:%02d\n", 
                                 year, month, day, hour, minute, second);
                }
                else
                {
                    Serial_Printf("[ERROR] Invalid time parameters. Check the ranges.\n");
                }
            }
            else
            {
                Serial_Printf("[ERROR] Invalid format. Use: time <YY> <MM> <DD> <HH> <mm> <SS>\n");
            }
        }
    }
    else if (strncmp(command, "history", 7) == 0)
        {
            // 解析history命令，查看历史记录
            int count = 10; // 默认显示10条记录
            if (command[7] == ' ' || command[7] == '\0')
            {
                if (strlen(command) > 7)
                {
                    count = atoi(command + 8);
                    if (count <= 0)
                    {
                        count = 10;
                    }
                    else if (count > MAX_RECORDS)
                    {
                        count = MAX_RECORDS;
                    }
                }
                
                // 读取记录
                DataRecord_t record;
                uint32_t total_records = (record_index > 0) ? record_index : MAX_RECORDS;
                uint32_t start_index = (record_index >= count) ? (record_index - count) : 0;
                uint32_t show_count = (record_index >= count) ? count : record_index;
                
                Serial_Printf("[HISTORY] Total records: %lu, Showing: %lu\n", total_records, show_count);
                Serial_Printf("[HISTORY] Time | Temp | Humi | Mode | IR\n");
                Serial_Printf("[HISTORY] ---- | ---- | ---- | ---- | --\n");
                
                for (uint32_t i = start_index; i < start_index + show_count; i++)
                {
                    uint8_t crc_result = W25Q64_ReadRecord(&record, i);
                    if (crc_result == 0) /* CRC match, record is valid */
                    {
                        RTC_TimeTypeDef rec_time;
                        RTC_ConvertFromSeconds(record.timestamp, &rec_time);
                        Serial_Printf("[HISTORY] 20%02d-%02d-%02d %02d:%02d:%02d | %4d | %4d | %4d | %2d\n", 
                                      rec_time.year, rec_time.month, rec_time.day, rec_time.hour, rec_time.minute, rec_time.second,
                                      record.temperature, 
                                      record.humidity, 
                                      record.system_mode, 
                                      record.ir_status);
                    }
                    else /* CRC mismatch, record is invalid */
                    {
                        Serial_Printf("[HISTORY] %4lu | INVALID DATA\n", i);
                    }
                }
            }
            else
            {
                Serial_Printf("[ERROR] Invalid history command. Use: history [count]\n");
            }
        }
        else if (strncmp(command, "export", 6) == 0)
        {
            // 导出数据为CSV格式
            DataRecord_t record;
            uint32_t total_records = (record_index > 0) ? record_index : MAX_RECORDS;
            
            Serial_Printf("[EXPORT] CSV format data (Records: %lu)\n", total_records);
            Serial_Printf("Timestamp,Temperature,Humidity,Mode,IR_Status\n");
            
            for (uint32_t i = 0; i < total_records; i++)
            {
                uint8_t crc_result = W25Q64_ReadRecord(&record, i);
                if (crc_result == 0) /* CRC match, record is valid */
                {
                    RTC_TimeTypeDef rec_time;
                    RTC_ConvertFromSeconds(record.timestamp, &rec_time);
                    Serial_Printf("20%02d-%02d-%02d %02d:%02d:%02d,%d,%d,%d,%d\n", 
                                 rec_time.year, rec_time.month, rec_time.day, rec_time.hour, rec_time.minute, rec_time.second,
                                 record.temperature, 
                                 record.humidity, 
                                 record.system_mode, 
                                 record.ir_status);
                }
                else /* CRC mismatch, record is invalid */
                {
                    Serial_Printf("%lu,INVALID,INVALID,INVALID,INVALID\n", i);
                }
            }
            
            Serial_Printf("[EXPORT] Data export completed\n");
        }
        else if (strncmp(command, "clear_history", 13) == 0)
        {
            // 清空历史记录
            W25Q64_ClearAllRecords();
            record_index = 0; // 重置记录索引
            /*保存重置后的索引*/
            W25Q64_WriteRecordIndex(record_index);
            Serial_Printf("[INFO] All historical data cleared\n");
        }
        else
        {
            Serial_Printf("[ERROR] Unknown command. Type 'help' for available commands\n");
        }
}