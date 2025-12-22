#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/*引脚配置*/
#define ENCODER_PORT GPIOB
#define ENCODER_A_PIN GPIO_Pin_0
#define ENCODER_B_PIN GPIO_Pin_1
#define ENCODER_KEY_PIN GPIO_Pin_10

/*编码器计数变量*/
int16_t Encoder_Count = 0;
uint8_t Encoder_KeyFlag = 0;

/*编码器防抖变量*/
static int16_t last_valid_diff = 0;
static uint8_t debounce_counter = 0;
#define DEBOUNCE_THRESHOLD 1 //只需要检测到一次旋转就有效

/*按键防抖变量*/
static uint8_t key_debounce_counter = 0;
static uint8_t key_state = 1; //初始状态为释放（高电平）
#define KEY_DEBOUNCE_THRESHOLD 5 //按键防抖阈值（5个主循环周期，约2.5秒）

/*编码器初始化*/
void Encoder_Init(void)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    /*GPIO初始化 - 编码器A/B相*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = ENCODER_A_PIN | ENCODER_B_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ENCODER_PORT, &GPIO_InitStructure);
    
    /*GPIO初始化 - 编码器按键*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = ENCODER_KEY_PIN;
    GPIO_Init(ENCODER_PORT, &GPIO_InitStructure);
    
    /*TIM3编码器模式初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 65535;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
    
    /*编码器模式配置*/
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    
    /*输入捕获滤波配置 - 增加滤波值，减少干扰*/
    TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 0x04; //适中的滤波值，平衡抗干扰和灵敏度
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
    
    /*外部中断配置 - 按键*/
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
    
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line10;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStructure);
    
    /*NVIC配置*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    /*TIM3使能*/
    TIM_Cmd(TIM3, ENABLE);
    
    /*初始化计数器值到中间位置，减少溢出影响*/
    TIM_SetCounter(TIM3, 32768);
}

/*获取编码器计数*/
int16_t Encoder_GetCount(void)
{
    int16_t count = Encoder_Count;
    Encoder_Count = 0;
    return count;
}

/*获取编码器按键状态*/
uint8_t Encoder_GetKeyStatus(void)
{
    if (Encoder_KeyFlag)
    {
        Encoder_KeyFlag = 0;
        return 1;
    }
    return 0;
}

/*定时器3中断函数 - 用于编码器计数更新*/
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

/*外部中断15_10中断函数 - 用于编码器按键检测*/
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line10) == SET)
    {
        //按键按下时产生中断，只设置标志位，防抖在主循环中处理
        Encoder_KeyFlag = 1;
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
}

/*编码器计数更新函数 - 在主循环中调用*/
void Encoder_Update(void)
{
    static uint16_t last_count = 32768; //初始化为中间值
    uint16_t current_count = TIM_GetCounter(TIM3);
    int16_t diff;
    
    //计算差值，处理16位无符号整数溢出
    if (current_count > last_count)
    {
        //顺时针旋转
        if (current_count - last_count > 32768)
        {
            //发生下溢，实际是逆时针旋转
            diff = -(65536 - (current_count - last_count));
        }
        else
        {
            diff = current_count - last_count;
        }
    }
    else
    {
        //逆时针旋转
        if (last_count - current_count > 32768)
        {
            //发生上溢，实际是顺时针旋转
            diff = 65536 - (last_count - current_count);
        }
        else
        {
            diff = -(last_count - current_count);
        }
    }
    
    //仅当差值超过一定阈值时才进行防抖处理
    if (abs(diff) >= 1) // 降低阈值到1，提高灵敏度
    {
        //检查当前旋转方向与上一次是否相同
        if ((diff > 0 && last_valid_diff > 0) || (diff < 0 && last_valid_diff < 0))
        {
            //方向相同，增加防抖计数器
            debounce_counter++;
        }
        else
        {
            //方向不同，重置防抖计数器
            debounce_counter = 1;
            last_valid_diff = diff;
        }
        
        //当连续检测到相同方向的旋转达到阈值时，才更新计数
        if (debounce_counter >= DEBOUNCE_THRESHOLD)
        {
            Encoder_Count += diff;
            last_count = current_count;
            debounce_counter = 0; //重置防抖计数器
        }
    }
    else
    {
        //差值太小，重置防抖计数器
        debounce_counter = 0;
    }
    
    //按键防抖处理
    uint8_t current_key_state = GPIO_ReadInputDataBit(ENCODER_PORT, ENCODER_KEY_PIN);
    
    if (current_key_state != key_state)
    {
        //按键状态变化，增加防抖计数器
        key_debounce_counter++;
        
        if (key_debounce_counter >= KEY_DEBOUNCE_THRESHOLD)
        {
            //状态稳定，更新按键状态
            key_state = current_key_state;
            key_debounce_counter = 0;
            
            //如果是按下状态（低电平），设置按键标志
            if (key_state == 0)
            {
                Encoder_KeyFlag = 1;
            }
        }
    }
    else
    {
        //状态未变化，重置防抖计数器
        key_debounce_counter = 0;
    }
}