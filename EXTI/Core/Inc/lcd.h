#ifndef LCD_H_
#define LCD_H_

#include "stm32f4xx_hal.h"
#include "main.h"

// Fonksiyon Prototipleri
void LCD_Init(void);
void LCD_Komut(uint8_t komut);
void LCD_Veri(uint8_t veri);
void LCD_Yaz(char *str);
void LCD_SatirGit(uint8_t satir);
void LCD_Temizle(void);

#endif /* LCD_H_ */
