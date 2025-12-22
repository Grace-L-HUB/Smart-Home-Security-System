/**
  ******************************************************************************
  * @file    W25Q64.c
  * @author  AI Assistant
  * @version V1.0.0
  * @date    2025-12-13
  * @brief   W25Q64 SPI Flash driver implementation
  ******************************************************************************
  */

#include "W25Q64.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "Serial.h"
#include <stddef.h>

/**
  * @brief  Initializes the W25Q64 SPI communication
  * @param  None
  * @retval None
  */
void W25Q64_Init(void)
{
    SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable SPI and GPIO clocks */
    RCC_APB2PeriphClockCmd(W25Q64_SPI_CLK | W25Q64_SPI_GPIO_CLK | W25Q64_CS_GPIO_CLK, ENABLE);

    /* Configure SPI pins: SCK, MISO, MOSI */
    GPIO_InitStructure.GPIO_Pin = W25Q64_SPI_PIN_SCK | W25Q64_SPI_PIN_MOSI;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(W25Q64_SPI_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = W25Q64_SPI_PIN_MISO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(W25Q64_SPI_GPIO_PORT, &GPIO_InitStructure);

    /* Configure CS pin */
    GPIO_InitStructure.GPIO_Pin = W25Q64_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(W25Q64_CS_GPIO_PORT, &GPIO_InitStructure);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* SPI configuration */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; /* 72MHz/16=4.5MHz */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    /* Initialize SPI */
    SPI_Init(W25Q64_SPI, &SPI_InitStructure);

    /* Enable SPI */
    SPI_Cmd(W25Q64_SPI, ENABLE);
}

/**
  * @brief  Sends a byte through the SPI interface and return the received byte
  * @param  data: byte to send
  * @retval The value of the received byte
  */
static uint8_t W25Q64_SPI_SendByte(uint8_t data)
{
    /* Wait for TXE flag to be set */
    while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_TXE) == RESET);

    /* Send byte through the SPI peripheral */
    SPI_I2S_SendData(W25Q64_SPI, data);

    /* Wait for RXNE flag to be set */
    while (SPI_I2S_GetFlagStatus(W25Q64_SPI, SPI_I2S_FLAG_RXNE) == RESET);

    /* Return the received byte */
    return SPI_I2S_ReceiveData(W25Q64_SPI);
}

/**
  * @brief  Reads the status register 1 of the W25Q64
  * @param  None
  * @retval Status register 1 value
  */
static uint8_t W25Q64_ReadStatusReg1(void)
{
    uint8_t status;

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Read Status Register 1 command */
    W25Q64_SPI_SendByte(W25Q64_CMD_READ_STATUS_REG1);

    /* Read status register value */
    status = W25Q64_SPI_SendByte(0x00);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    return status;
}

/**
  * @brief  Waits until the W25Q64 is ready for a new operation
  * @param  None
  * @retval None
  */
static void W25Q64_WaitForReady(void)
{
    /* Wait until BUSY bit is cleared */
    while ((W25Q64_ReadStatusReg1() & W25Q64_SR1_BUSY) == W25Q64_SR1_BUSY);
}

/**
  * @brief  Sends Write Enable command to the W25Q64
  * @param  None
  * @retval None
  */
static void W25Q64_WriteEnable(void)
{
    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Write Enable command */
    W25Q64_SPI_SendByte(W25Q64_CMD_WRITE_ENABLE);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();
}

/**
  * @brief  Reads a single byte from the W25Q64 at the specified address
  * @param  addr: address to read from
  * @retval Byte read from the W25Q64
  */
uint8_t W25Q64_ReadByte(uint32_t addr)
{
    uint8_t data;

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Read Data command */
    W25Q64_SPI_SendByte(W25Q64_CMD_READ_DATA);

    /* Send address */
    W25Q64_SPI_SendByte((addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(addr & 0xFF);

    /* Read data */
    data = W25Q64_SPI_SendByte(0x00);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    return data;
}

/**
  * @brief  Reads multiple bytes from the W25Q64 starting at the specified address
  * @param  addr: start address to read from
  * @param  buffer: pointer to buffer to store read data
  * @param  length: number of bytes to read
  * @retval None
  */
void W25Q64_ReadBytes(uint32_t addr, uint8_t* buffer, uint32_t length)
{
    uint32_t i;

    /* Check parameters */
    if (buffer == NULL || length == 0) return;

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Read Data command */
    W25Q64_SPI_SendByte(W25Q64_CMD_READ_DATA);

    /* Send address */
    W25Q64_SPI_SendByte((addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(addr & 0xFF);

    /* Read data */
    for (i = 0; i < length; i++)
    {
        buffer[i] = W25Q64_SPI_SendByte(0x00);
    }

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();
}

/**
  * @brief  Writes a single byte to the W25Q64 at the specified address
  * @param  addr: address to write to
  * @param  data: byte to write
  * @retval None
  */
void W25Q64_WriteByte(uint32_t addr, uint8_t data)
{
    /* Wait for W25Q64 to be ready */
    W25Q64_WaitForReady();

    /* Send Write Enable command */
    W25Q64_WriteEnable();

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Page Program command */
    W25Q64_SPI_SendByte(W25Q64_CMD_PAGE_PROGRAM);

    /* Send address */
    W25Q64_SPI_SendByte((addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(addr & 0xFF);

    /* Send data */
    W25Q64_SPI_SendByte(data);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* Wait for write to complete */
    W25Q64_WaitForReady();
}

/**
  * @brief  Writes multiple bytes to the W25Q64 starting at the specified address
  * @param  addr: start address to write to
  * @param  buffer: pointer to buffer containing data to write
  * @param  length: number of bytes to write
  * @retval None
  */
void W25Q64_WriteBytes(uint32_t addr, uint8_t* buffer, uint32_t length)
{
    uint32_t current_page, next_page_start, bytes_to_write;
    uint32_t i = 0;

    /* Check parameters */
    if (buffer == NULL || length == 0) return;

    while (length > 0)
    {
        /* Wait for W25Q64 to be ready */
        W25Q64_WaitForReady();

        /* Calculate current page and next page start address */
        current_page = addr / W25Q64_PAGE_SIZE;
        next_page_start = (current_page + 1) * W25Q64_PAGE_SIZE;

        /* Calculate bytes to write in current page */
        bytes_to_write = next_page_start - addr;
        if (bytes_to_write > length)
        {
            bytes_to_write = length;
        }

        /* Send Write Enable command */
        W25Q64_WriteEnable();

        /* Select W25Q64 */
        W25Q64_CS_LOW();

        /* Send Page Program command */
        W25Q64_SPI_SendByte(W25Q64_CMD_PAGE_PROGRAM);

        /* Send address */
        W25Q64_SPI_SendByte((addr >> 16) & 0xFF);
        W25Q64_SPI_SendByte((addr >> 8) & 0xFF);
        W25Q64_SPI_SendByte(addr & 0xFF);

        /* Send data */
        for (i = 0; i < bytes_to_write; i++)
        {
            W25Q64_SPI_SendByte(buffer[i]);
        }

        /* Deselect W25Q64 */
        W25Q64_CS_HIGH();

        /* Update address, buffer and remaining length */
        addr += bytes_to_write;
        buffer += bytes_to_write;
        length -= bytes_to_write;

        /* Wait for write to complete */
        W25Q64_WaitForReady();
    }
}

/**
  * @brief  Erases a sector (4KB) of the W25Q64 at the specified address
  * @param  sector_addr: sector address to erase
  * @retval None
  */
void W25Q64_EraseSector(uint32_t sector_addr)
{
    /* Wait for W25Q64 to be ready */
    W25Q64_WaitForReady();

    /* Send Write Enable command */
    W25Q64_WriteEnable();

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Sector Erase command */
    W25Q64_SPI_SendByte(W25Q64_CMD_SECTOR_ERASE_4KB);

    /* Send sector address */
    W25Q64_SPI_SendByte((sector_addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((sector_addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(sector_addr & 0xFF);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* Wait for erase to complete */
    W25Q64_WaitForReady();
}

/**
  * @brief  Erases a 32KB block of the W25Q64 at the specified address
  * @param  block_addr: block address to erase
  * @retval None
  */
void W25Q64_EraseBlock32K(uint32_t block_addr)
{
    /* Wait for W25Q64 to be ready */
    W25Q64_WaitForReady();

    /* Send Write Enable command */
    W25Q64_WriteEnable();

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Block Erase 32KB command */
    W25Q64_SPI_SendByte(W25Q64_CMD_BLOCK_ERASE_32KB);

    /* Send block address */
    W25Q64_SPI_SendByte((block_addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((block_addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(block_addr & 0xFF);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* Wait for erase to complete */
    W25Q64_WaitForReady();
}

/**
  * @brief  Erases a 64KB block of the W25Q64 at the specified address
  * @param  block_addr: block address to erase
  * @retval None
  */
void W25Q64_EraseBlock64K(uint32_t block_addr)
{
    /* Wait for W25Q64 to be ready */
    W25Q64_WaitForReady();

    /* Send Write Enable command */
    W25Q64_WriteEnable();

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Block Erase 64KB command */
    W25Q64_SPI_SendByte(W25Q64_CMD_BLOCK_ERASE_64KB);

    /* Send block address */
    W25Q64_SPI_SendByte((block_addr >> 16) & 0xFF);
    W25Q64_SPI_SendByte((block_addr >> 8) & 0xFF);
    W25Q64_SPI_SendByte(block_addr & 0xFF);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* Wait for erase to complete */
    W25Q64_WaitForReady();
}

/**
  * @brief  Erases the entire W25Q64 chip
  * @param  None
  * @retval None
  */
void W25Q64_EraseChip(void)
{
    /* Wait for W25Q64 to be ready */
    W25Q64_WaitForReady();

    /* Send Write Enable command */
    W25Q64_WriteEnable();

    /* Select W25Q64 */
    W25Q64_CS_LOW();

    /* Send Chip Erase command */
    W25Q64_SPI_SendByte(W25Q64_CMD_CHIP_ERASE);

    /* Deselect W25Q64 */
    W25Q64_CS_HIGH();

    /* Wait for erase to complete (this may take several seconds) */
    W25Q64_WaitForReady();
}

/**
  * @brief  Writes a data record to the W25Q64 at the specified index
  * @param  record: pointer to DataRecord_t structure to write
  * @param  index: index of the record (0-based)
  * @retval None
  */
void W25Q64_WriteRecord(DataRecord_t* record, uint32_t index)
{
    uint32_t addr;
    DataRecord_t temp_record;
    
    /* Copy record data */
    temp_record = *record;
    
    /* Calculate CRC for the record data (excluding the CRC field itself) */
    temp_record.crc = W25Q64_CalculateCRC16((uint8_t*)&temp_record.timestamp, 
                                          sizeof(temp_record) - sizeof(temp_record.crc));
    
    /* Calculate record address */
    addr = index * sizeof(DataRecord_t);
    
    /* Write record to W25Q64 */
    W25Q64_WriteBytes(addr, (uint8_t*)&temp_record, sizeof(DataRecord_t));
}

/**
  * @brief  Reads a data record from the W25Q64 at the specified index
  * @param  record: pointer to DataRecord_t structure to store read data
  * @param  index: index of the record (0-based)
  * @retval uint8_t: 0 if record is valid (CRC match), 1 if CRC mismatch
  */
uint8_t W25Q64_ReadRecord(DataRecord_t* record, uint32_t index)
{
    uint32_t addr;
    uint16_t calculated_crc;
    DataRecord_t temp_record;

    /* Calculate record address */
    addr = index * sizeof(DataRecord_t);

    /* Read record from W25Q64 */
    W25Q64_ReadBytes(addr, (uint8_t*)&temp_record, sizeof(DataRecord_t));
    
    /* Calculate CRC for the received data (excluding the CRC field itself) */
    calculated_crc = W25Q64_CalculateCRC16((uint8_t*)&temp_record.timestamp, 
                                          sizeof(DataRecord_t) - sizeof(temp_record.crc));
    

    
    /* Copy to output record */
    *record = temp_record;
    
    /* Compare calculated CRC with the stored CRC */
    if (calculated_crc != temp_record.crc)
    {
        /* CRC mismatch, record is invalid */
        return 1;
    }
    
    /* CRC match, record is valid */
    return 0;
}

/**
  * @brief  Gets the total number of data records that can be stored in the W25Q64
  * @param  None
  * @retval Total number of data records
  */
uint32_t W25Q64_GetTotalRecords(void)
{
    /* Calculate total records based on W25Q64 size and record size */
    return W25Q64_TOTAL_SIZE / sizeof(DataRecord_t);
}

/**
  * @brief  Clears all data records by erasing the entire W25Q64 chip
  * @param  None
  * @retval None
  */
void W25Q64_ClearAllRecords(void)
{
    /* Erase the entire chip */
    W25Q64_EraseChip();
}

/**
  * @brief  Simple delay function
  * @param  nCount: delay counter
  * @retval None
  */
void W25Q64_Delay(uint32_t nCount)
{
    while(nCount--);
}

/**
  * @brief  Writes the record index to the W25Q64
  * @param  index: Record index to write
  * @retval None
  */
void W25Q64_WriteRecordIndex(uint32_t index)
{
    uint32_t addr = W25Q64_RECORD_INDEX_ADDR;
    uint8_t data[4];
    
    /* Convert index to byte array */
    data[0] = (index >> 24) & 0xFF;
    data[1] = (index >> 16) & 0xFF;
    data[2] = (index >> 8) & 0xFF;
    data[3] = index & 0xFF;
    
    /* Erase the sector containing the index (last sector of the chip) */
    W25Q64_EraseSector(addr - (addr % W25Q64_SECTOR_SIZE));
    
    /* Write the index */
    W25Q64_WriteBytes(addr, data, sizeof(data));
}

/**
  * @brief  Reads the record index from the W25Q64
  * @param  None
  * @retval Record index
  */
uint32_t W25Q64_ReadRecordIndex(void)
{
    uint32_t addr = W25Q64_RECORD_INDEX_ADDR;
    uint8_t data[4];
    uint32_t index;
    
    /* Read the index bytes */
    W25Q64_ReadBytes(addr, data, sizeof(data));
    
    /* Convert byte array to index */
    index = ((uint32_t)data[0] << 24) |
            ((uint32_t)data[1] << 16) |
            ((uint32_t)data[2] << 8) |
            data[3];
    
    return index;
}

/**
  * @brief  Calculates CRC16 checksum for data reliability
  * @param  data: Pointer to data buffer
  * @param  length: Length of data buffer
  * @retval CRC16 checksum
  */
uint16_t W25Q64_CalculateCRC16(const uint8_t* data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    uint32_t i, j;
    
    for (i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/**
  * @brief  Writes system configuration to the W25Q64
  * @param  config: Pointer to SystemConfig_t structure to write
  * @retval None
  */
void W25Q64_WriteConfig(SystemConfig_t* config)
{
    uint32_t addr = W25Q64_CONFIG_ADDR;
    SystemConfig_t temp_config;
    
    /* Copy config data */
    temp_config = *config;
    
    /* Calculate CRC for the config data (excluding the CRC field itself) */
    temp_config.crc = W25Q64_CalculateCRC16((uint8_t*)&temp_config.temp_threshold_low, 
                                          sizeof(temp_config) - sizeof(temp_config.crc));
    
    /* Erase the sector containing the config */
    W25Q64_EraseSector(addr - (addr % W25Q64_SECTOR_SIZE));
    
    /* Write config to W25Q64 */
    W25Q64_WriteBytes(addr, (uint8_t*)&temp_config, sizeof(SystemConfig_t));
}

/**
  * @brief  Reads system configuration from the W25Q64
  * @param  config: Pointer to SystemConfig_t structure to store read data
  * @retval uint8_t: 0 if config is valid (CRC match), 1 if CRC mismatch
  */
uint8_t W25Q64_ReadConfig(SystemConfig_t* config)
{
    uint32_t addr = W25Q64_CONFIG_ADDR;
    uint16_t calculated_crc;
    
    /* Read config from W25Q64 */
    W25Q64_ReadBytes(addr, (uint8_t*)config, sizeof(SystemConfig_t));
    
    /* Calculate CRC for the received data (excluding the CRC field itself) */
    calculated_crc = W25Q64_CalculateCRC16((uint8_t*)&config->temp_threshold_low, 
                                          sizeof(SystemConfig_t) - sizeof(config->crc));
    
    /* Compare calculated CRC with the stored CRC */
    if (calculated_crc != config->crc)
    {
        /* CRC mismatch, config is invalid */
        return 1;
    }
    
    /* CRC match, config is valid */
    return 0;
}