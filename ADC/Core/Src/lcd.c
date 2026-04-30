#include "lcd.h"
#include "main.h"

void LCD_Send4Bits(uint8_t data)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, (data >> 0) & 1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (data >> 1) & 1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, (data >> 2) & 1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, (data >> 3) & 1);
}

void LCD_SendCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);

    LCD_Send4Bits(cmd >> 4);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);

    LCD_Send4Bits(cmd & 0x0F);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
}

void LCD_SendData(uint8_t data)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);

    LCD_Send4Bits(data >> 4);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);

    LCD_Send4Bits(data & 0x0F);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
}

void LCD_Init(void)
{
    HAL_Delay(50);

    LCD_SendCommand(0x02);
    LCD_SendCommand(0x28);
    LCD_SendCommand(0x0C);
    LCD_SendCommand(0x06);
    LCD_SendCommand(0x01);

    HAL_Delay(2);
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if(row == 0)
        addr = 0x80 + col;
    else
        addr = 0xC0 + col;

    LCD_SendCommand(addr);
}

void LCD_Print(char *str)
{
    while(*str)
    {
        LCD_SendData(*str++);
    }
}
