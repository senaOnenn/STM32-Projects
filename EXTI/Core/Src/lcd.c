#include "lcd.h"

void LCD_PinYaz(uint8_t veri) {
    HAL_GPIO_WritePin(GPIOC, LCD_D4_Pin, (veri >> 0) & 0x01);
    HAL_GPIO_WritePin(GPIOC, LCD_D5_Pin, (veri >> 1) & 0x01);
    HAL_GPIO_WritePin(GPIOC, LCD_D6_Pin, (veri >> 2) & 0x01);
    HAL_GPIO_WritePin(GPIOC, LCD_D7_Pin, (veri >> 3) & 0x01);
}

void LCD_Enable(void) {
    HAL_GPIO_WritePin(GPIOD, LCD_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOD, LCD_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

void LCD_Komut(uint8_t komut) {
    HAL_GPIO_WritePin(GPIOD, LCD_RS_Pin, GPIO_PIN_RESET);
    LCD_PinYaz(komut >> 4);
    LCD_Enable();
    LCD_PinYaz(komut & 0x0F);
    LCD_Enable();
}

void LCD_Veri(uint8_t veri) {
    HAL_GPIO_WritePin(GPIOD, LCD_RS_Pin, GPIO_PIN_SET);
    LCD_PinYaz(veri >> 4);
    LCD_Enable();
    LCD_PinYaz(veri & 0x0F);
    LCD_Enable();
}

void LCD_Init(void) {
    HAL_Delay(50);
    LCD_Komut(0x02);
    LCD_Komut(0x28);
    LCD_Komut(0x0C);
    LCD_Komut(0x06);
    LCD_Komut(0x01);
    HAL_Delay(2);
}

void LCD_Yaz(char *str) {
    while (*str) LCD_Veri(*str++);
}

void LCD_SatirGit(uint8_t satir) {
    if (satir == 1) LCD_Komut(0x80);
    else if (satir == 2) LCD_Komut(0xC0);
}

void LCD_Temizle(void) {
    LCD_Komut(0x01);
    HAL_Delay(2);
}
