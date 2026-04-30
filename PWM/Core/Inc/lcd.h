#ifndef LCD_H_
#define LCD_H_
#include "stm32f4xx_hal.h"
// --- ArmApp-18 LCD Pin Tanımlamaları ---
// Kontrol Pinleri
#define RS_PORT GPIOD
#define RS_PIN GPIO_PIN_2
#define EN_PORT GPIOD
#define EN_PIN GPIO_PIN_3
// Veri (Data) Pinleri - 4 Bit Mod
#define D4_PORT GPIOC
#define D4_PIN GPIO_PIN_4
#define D5_PORT GPIOC
#define D5_PIN GPIO_PIN_13
#define D6_PORT GPIOC
#define D6_PIN GPIO_PIN_14
#define D7_PORT GPIOC
#define D7_PIN GPIO_PIN_15
// --- Fonksiyon Prototipleri ---
void Lcd_Init(void); // Ekranı başlatır
void Lcd_Cmd(uint8_t cmd); // Komut gönderir
void Lcd_Char(char data); // Tek karakter yazar
void Lcd_String(char *str); // Metin yazar
void Lcd_Set_Cursor(uint8_t row, uint8_t col); // İmleç konumlandırma
void Lcd_Clear(void); // Ekranı temizler
#endif
