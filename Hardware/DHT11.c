#include "dht11.h" 
#include "delay.h" 
#include "stdio.h" 
#include "Serial.h" 
		 
void DHT11_Rst(void)	   //复位DHT11 
{ 
	//SET OUTPUT 
	DHT11_Mode(OUT); 
	//拉低DQ 
	DHT11_Low; 
	//主机拉低18~30ms 
	Delay_ms(20); 
	//拉高DQ 
	DHT11_High; 
	//主机拉高10~35us 
	Delay_us(13);      	 
} 
  

uint8_t DHT11_Check(void) 	   
{   
	uint8_t retry=0; 
	DHT11_Mode(IN);//SET INPUT	 
    while (GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//DHT11会拉低40~80us 
	{ 
		retry++; 
		Delay_us(1); 
	};    
	if(retry>=100)return 1; 
	else retry=0; 
    while (!GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//DHT11拉低后会再次拉高40~80us 
	{ 
		retry++; 
		Delay_us(1); 
	}; 
	if(retry>=100)return 1;    
	return 0; 
} 

uint8_t DHT11_ReadBit(void) 			 
{ 
  uint8_t retry=0; 
  while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//等待变为低电平 
  { retry++; Delay_us(1); } 
  retry=0; 
  while(!GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//等待变高电平 
  { retry++; Delay_us(1); } 
  Delay_us(30);//30us延时区分0/1位，DHT11 0位约26-28us，1位约70us 
  if(GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN))return 1; 
  else return 0;		   
} 
  

/*========================================================== 
Name：	DHT11_ReadByte 
Function：	读取DHT11一个字节 
pars:		无 
return：	读到的数据 
notes：		无 
==========================================================*/ 
uint8_t DHT11_ReadByte(void)    
{        
	uint8_t i,dat; 
	dat=0; 
	for (i=0;i<8;i++) 
	{ 
		dat<<=1; 
		dat|=DHT11_ReadBit(); 
	}						    
	return dat; 
} 

uint8_t DHT11_ReadData(uint8_t *humi, uint8_t *temp)    // 修正参数顺序，先湿度后温度
{
  uint8_t buff[5]; 
  uint8_t i; 
  DHT11_Rst(); //发送起始信号 
  if(DHT11_Check()==0) //等待响应 
  {
    for(i=0;i<5;i++)//读取40位数据 
    {
      buff[i]=DHT11_ReadByte(); 
    }
    
    // 计算并验证校验和
    uint8_t checksum = buff[0] + buff[1] + buff[2] + buff[3];
    if(checksum == buff[4]) 
    {
      *humi=buff[0];		//湿度数据 
      *temp=buff[2];		//温度数据 
    }
    
    // 打印原始数据以便调试
    printf("[DEBUG] DHT11 Raw Data: %02X %02X %02X %02X %02X\n", 
           buff[0], buff[1], buff[2], buff[3], buff[4]);
    printf("[DEBUG] DHT11 Checksum: Calculated=%02X, Received=%02X\n", 
           checksum, buff[4]);
    printf("[DEBUG] DHT11 Extracted: Temp=%d, Humi=%d\n", 
           *temp, *humi);
  } 
  else return 1; 
  return 0;	    
} 
  
/*========================================================== 
Name：	DHT11_Init 
Function：	初始化DHT11的IO口，同时检测DHT11的存在 
pars:		temp:温度 
		humi:湿度 
return：	0-存在 
		1-不存在 
notes：		无 
==========================================================*/ 
uint8_t DHT11_Init(void) 
{	 
  GPIO_InitTypeDef  GPIO_InitStructure;	 
  RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);	 	//使能PA端口时钟 
  GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;		//端口配置 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//推挽输出 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);	//初始化IO口 
  GPIO_SetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN);		//输出高电平 
		    
	DHT11_Rst();  //发送起始信号 
	return DHT11_Check();//等待DHT11的回应 
} 
  
void DHT11_Mode(uint8_t mode) 
{ 
	GPIO_InitTypeDef GPIO_InitStructure; 
	 
	if(mode) 
	{ 
		GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN; 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	} 
	else 
	{ 
		GPIO_InitStructure.GPIO_Pin =  DHT11_GPIO_PIN; 
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	} 
	GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure); 
}

