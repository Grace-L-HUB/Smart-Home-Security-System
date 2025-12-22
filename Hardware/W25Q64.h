/**
  ******************************************************************************
  * @file    W25Q64.h
  * @author  AI Assistant
  * @version V1.0.0
  * @date    2025-12-13
  * @brief   W25Q64 SPI Flash driver header file
  ******************************************************************************
  */

#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"

/* W25Q64 SPI Flash commands */
#define W25Q64_CMD_WRITE_ENABLE         0x06  /* Write Enable */
#define W25Q64_CMD_WRITE_DISABLE        0x04  /* Write Disable */
#define W25Q64_CMD_READ_STATUS_REG1     0x05  /* Read Status Register 1 */
#define W25Q64_CMD_READ_STATUS_REG2     0x35  /* Read Status Register 2 */
#define W25Q64_CMD_WRITE_STATUS_REG     0x01  /* Write Status Register */
#define W25Q64_CMD_PAGE_PROGRAM         0x02  /* Page Program */
#define W25Q64_CMD_QUAD_PAGE_PROGRAM    0x32  /* Quad Page Program */
#define W25Q64_CMD_BLOCK_ERASE_64KB     0xD8  /* Block Erase 64KB */
#define W25Q64_CMD_BLOCK_ERASE_32KB     0x52  /* Block Erase 32KB */
#define W25Q64_CMD_SECTOR_ERASE_4KB     0x20  /* Sector Erase 4KB */
#define W25Q64_CMD_CHIP_ERASE           0xC7  /* Chip Erase */
#define W25Q64_CMD_ERASE_SUSPEND        0x75  /* Erase Suspend */
#define W25Q64_CMD_ERASE_RESUME         0x7A  /* Erase Resume */
#define W25Q64_CMD_POWER_DOWN           0xB9  /* Power Down */
#define W25Q64_CMD_HIGH_PERFORMANCE_MODE 0xA3 /* High Performance Mode */
#define W25Q64_CMD_CONTINUOUS_READ_MODE_RESET 0xFF /* Continuous Read Mode Reset */
#define W25Q64_CMD_RELEASE_POWER_DOWN   0xAB  /* Release Power Down / Device ID */
#define W25Q64_CMD_MANUFACTURER_DEVICE_ID 0x90 /* Manufacturer / Device ID */
#define W25Q64_CMD_READ_UNIQUE_ID       0x4B  /* Read Unique ID */
#define W25Q64_CMD_JEDEC_ID             0x9F  /* JEDEC ID */
#define W25Q64_CMD_READ_DATA            0x03  /* Read Data */
#define W25Q64_CMD_FAST_READ            0x0B  /* Fast Read */
#define W25Q64_CMD_FAST_READ_DUAL_OUTPUT 0x3B  /* Fast Read Dual Output */
#define W25Q64_CMD_FAST_READ_DUAL_IO    0xBB  /* Fast Read Dual I/O */
#define W25Q64_CMD_FAST_READ_QUAD_OUTPUT 0x6B /* Fast Read Quad Output */
#define W25Q64_CMD_FAST_READ_QUAD_IO    0xEB  /* Fast Read Quad I/O */
#define W25Q64_CMD_OCTAL_WORD_READ      0xE3  /* Octal Word Read */

/* W25Q64 device information */
#define W25Q64_PAGE_SIZE                256   /* 256 bytes per page */
#define W25Q64_SECTOR_SIZE              4096  /* 4KB per sector */
#define W25Q64_BLOCK_32KB_SIZE          32768 /* 32KB per block */
#define W25Q64_BLOCK_64KB_SIZE          65536 /* 64KB per block */
#define W25Q64_TOTAL_SIZE               0x800000 /* 8MB total size */
#define W25Q64_NUM_PAGES                (W25Q64_TOTAL_SIZE / W25Q64_PAGE_SIZE) /* 32768 pages */
#define W25Q64_NUM_SECTORS              (W25Q64_TOTAL_SIZE / W25Q64_SECTOR_SIZE) /* 2048 sectors */

/* W25Q64 Status Register 1 bits */
#define W25Q64_SR1_BUSY                 ((uint8_t)0x01) /* Busy bit */
#define W25Q64_SR1_WEL                  ((uint8_t)0x02) /* Write Enable Latch bit */
#define W25Q64_SR1_BP0                  ((uint8_t)0x04) /* Block Protect 0 */
#define W25Q64_SR1_BP1                  ((uint8_t)0x08) /* Block Protect 1 */
#define W25Q64_SR1_BP2                  ((uint8_t)0x10) /* Block Protect 2 */
#define W25Q64_SR1_TB                   ((uint8_t)0x20) /* Top/Bottom Protect */
#define W25Q64_SR1_SEC                  ((uint8_t)0x40) /* Sector Protect */
#define W25Q64_SR1_SRP0                 ((uint8_t)0x80) /* Status Register Protect 0 */

/* W25Q64 Status Register 2 bits */
#define W25Q64_SR2_SRP1                 ((uint8_t)0x01) /* Status Register Protect 1 */
#define W25Q64_SR2_QE                   ((uint8_t)0x02) /* Quad Enable */
#define W25Q64_SR2_LB0                  ((uint8_t)0x04) /* Security Register Lock 0 */
#define W25Q64_SR2_LB1                  ((uint8_t)0x08) /* Security Register Lock 1 */
#define W25Q64_SR2_LB2                  ((uint8_t)0x10) /* Security Register Lock 2 */
#define W25Q64_SR2_CMP                  ((uint8_t)0x20) /* Complement Protect */
#define W25Q64_SR2_SUS                  ((uint8_t)0x40) /* Suspend Status */

/* W25Q64 JEDEC ID */
#define W25Q64_JEDEC_MANUFACTURER_ID    0xEF  /* Manufacturer ID */
#define W25Q64_JEDEC_DEVICE_ID          0x16  /* Device ID for W25Q64 */

/* W25Q64 SPI configuration */
#define W25Q64_SPI                      SPI1
#define W25Q64_SPI_CLK                  RCC_APB2Periph_SPI1
#define W25Q64_SPI_GPIO_PORT            GPIOA
#define W25Q64_SPI_GPIO_CLK             RCC_APB2Periph_GPIOA
#define W25Q64_SPI_PIN_SCK              GPIO_Pin_5
#define W25Q64_SPI_PIN_MISO             GPIO_Pin_6
#define W25Q64_SPI_PIN_MOSI             GPIO_Pin_7

/* W25Q64 CS pin configuration */
#define W25Q64_CS_GPIO_PORT             GPIOA
#define W25Q64_CS_GPIO_CLK              RCC_APB2Periph_GPIOA
#define W25Q64_CS_PIN                   GPIO_Pin_4

/* W25Q64 CS pin control macros */
#define W25Q64_CS_LOW()                 GPIO_ResetBits(W25Q64_CS_GPIO_PORT, W25Q64_CS_PIN)
#define W25Q64_CS_HIGH()                GPIO_SetBits(W25Q64_CS_GPIO_PORT, W25Q64_CS_PIN)

/* Data Record structure for sensor data storage with CRC */
#pragma pack(1) /* 强制1字节对齐，避免填充字节导致CRC计算错误 */
typedef struct {
    uint32_t timestamp;      /* Timestamp (seconds since system start) */
    uint8_t temperature;     /* Temperature value (°C) */
    uint8_t humidity;        /* Humidity value (%) */
    uint8_t system_mode;     /* System mode (0: armed, 1: home, 2: debug) */
    uint8_t ir_status;       /* IR sensor status */
    uint16_t crc;            /* CRC16 checksum for data integrity */
} DataRecord_t;

/* System Configuration structure for persistent storage */
typedef struct {
    uint8_t temp_threshold_low;  /* Temperature lower threshold (°C) */
    uint8_t temp_threshold_high; /* Temperature upper threshold (°C) */
    uint8_t humi_threshold_low;  /* Humidity lower threshold (%) */
    uint8_t humi_threshold_high; /* Humidity upper threshold (%) */
    uint16_t crc;                /* CRC16 checksum for data integrity */
} SystemConfig_t;
#pragma pack() /* 恢复默认对齐 */

/* W25Q64 function prototypes */
void W25Q64_Init(void);
uint8_t W25Q64_ReadByte(uint32_t addr);
void W25Q64_ReadBytes(uint32_t addr, uint8_t* buffer, uint32_t length);
void W25Q64_WriteByte(uint32_t addr, uint8_t data);
void W25Q64_WriteBytes(uint32_t addr, uint8_t* buffer, uint32_t length);
void W25Q64_EraseSector(uint32_t sector_addr);
void W25Q64_EraseBlock32K(uint32_t block_addr);
void W25Q64_EraseBlock64K(uint32_t block_addr);
void W25Q64_EraseChip(void);
void W25Q64_WriteRecord(DataRecord_t* record, uint32_t index);
uint8_t W25Q64_ReadRecord(DataRecord_t* record, uint32_t index);
uint32_t W25Q64_GetTotalRecords(void);
void W25Q64_ClearAllRecords(void);
void W25Q64_Delay(uint32_t nCount);

/* Record Index functions */
#define W25Q64_RECORD_INDEX_ADDR        (W25Q64_TOTAL_SIZE - sizeof(uint32_t)) /* Store index at the end of flash */
void W25Q64_WriteRecordIndex(uint32_t index);
uint32_t W25Q64_ReadRecordIndex(void);

/* System Configuration functions */
#define W25Q64_CONFIG_ADDR              (W25Q64_RECORD_INDEX_ADDR - sizeof(SystemConfig_t)) /* Store config before index */
void W25Q64_WriteConfig(SystemConfig_t* config);
uint8_t W25Q64_ReadConfig(SystemConfig_t* config);

/* CRC functions for data reliability */
uint16_t W25Q64_CalculateCRC16(const uint8_t* data, uint32_t length);

#endif /* __W25Q64_H */