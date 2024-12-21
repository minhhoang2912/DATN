#include "flash.h"
#include "stm32f1xx.h"                  // Device header

uint8_t lengthPage;

/***********************************WRITE FLASH********************************************/

void deleteBuffer(char* data)
{
    uint8_t len = strlen(data);
    for(uint8_t i = 0; i < len; i++)
    {
        data[i] = 0;
    }
}

void Flash_Erase(uint32_t address)
{
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseIntStruct;
    EraseIntStruct.Banks = 1;
    EraseIntStruct.NbPages = 1;
    EraseIntStruct.PageAddress = address;
    EraseIntStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    uint32_t pageerr;
    HAL_FLASHEx_Erase(&EraseIntStruct, &pageerr);
    HAL_FLASH_Lock();
}

void Flash_Write_Int(uint32_t address, int value)
{
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, value);
    HAL_FLASH_Lock();
}
void Flash_Write_Float(uint32_t address, float f)
{
    HAL_FLASH_Unlock();
    uint8_t data[4];
    *(float*)data = f;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t*)data);
    HAL_FLASH_Lock();
}

void Flash_Write_Array(uint32_t address, uint16_t *arr, uint16_t len)
{
    HAL_FLASH_Unlock();
    uint16_t *pt = (uint16_t*)arr;
    for(uint16_t i=0; i<(len+1)/2; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + 2*i, *pt);
        pt++;
    }
    HAL_FLASH_Lock();
}

void Flash_Write_Struct(uint32_t address, wifi_info_t data)
{
    Flash_Write_Array(address, (uint16_t*)&data, sizeof(data));
}


void Flash_Write_Char(uint32_t addr, char* data)
{
    HAL_FLASH_Lock();
    int i;
    FLASH->CR |= FLASH_CR_PG;
    int var = 0;
    lengthPage = strlen(data);
    for(i=0; i<lengthPage; i+=1)
    {
        while((FLASH->SR&FLASH_SR_BSY));
        var = (int)data[i];
        *(__IO uint16_t*)(addr + i*2) = var;
    }
    while((FLASH->SR&FLASH_SR_BSY)) {};
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |= FLASH_CR_LOCK;
}

/***************************************READ FLASH******************************************/
int Flash_Read_Int(uint32_t address)
{
    return *(__IO uint16_t *)(address);
}

float Flash_Read_Float(uint32_t address)
{
    uint32_t data = *(__IO uint32_t*)(address);
    return *(float*)(&data);
}

void Flash_Read_Array(uint32_t address, uint16_t *arr, uint16_t len)
{
    for(uint16_t i=0; i<(len+1)/2; i++)
    {
        *arr = *(__IO uint16_t*)(address + 2*i);
        arr++;
    }
}
void Flash_Read_Struct(uint32_t address, wifi_info_t data)
{
    Flash_Read_Array(address, (uint16_t*)&data, sizeof(data));
}

void Flash_ReadChar(char* dataOut, uint32_t addr1, uint32_t addr2)
{
    int check = 0;
    deleteBuffer(dataOut);
    if((unsigned char)Flash_Read_Int(addr2+(uint32_t)2) == 255)
    {
        check = (unsigned char)Flash_Read_Int(addr2)-48;
    }
    else
    {
        check = ((unsigned char)Flash_Read_Int(addr2)-48)*10 + (unsigned char)Flash_Read_Int(addr2+2)-48;
    }
    for(int i = 0; i < check; i++)
    {
        dataOut[i] = Flash_Read_Int(addr1 + (uint32_t)(i*2));
    }
}