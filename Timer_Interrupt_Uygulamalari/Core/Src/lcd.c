#include "lcd.h"
// Yardımcı Fonksiyon: 4-Bit veriyi ilgili pinlere dağıtır
void Lcd_Port(uint8_t data)
{
HAL_GPIO_WritePin(D4_PORT, D4_PIN, (data & 0x01) ? GPIO_PIN_SET :
GPIO_PIN_RESET);
HAL_GPIO_WritePin(D5_PORT, D5_PIN, (data & 0x02) ? GPIO_PIN_SET :
GPIO_PIN_RESET);
HAL_GPIO_WritePin(D6_PORT, D6_PIN, (data & 0x04) ? GPIO_PIN_SET :
GPIO_PIN_RESET);
HAL_GPIO_WritePin(D7_PORT, D7_PIN, (data & 0x08) ? GPIO_PIN_SET :
GPIO_PIN_RESET);
}
// LCD'ye Komut Gönderme (RS=0)
void Lcd_Cmd(uint8_t cmd)
{
HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_RESET); // Komut Modu
Lcd_Port(cmd >> 4); // Üst 4 bit gönder
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
HAL_Delay(1);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
HAL_Delay(1);
Lcd_Port(cmd); // Alt 4 bit gönder
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
HAL_Delay(1);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
HAL_Delay(2);
}
// LCD'ye Karakter Yazma (RS=1)
void Lcd_Char(char data)
{
HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_SET); // Veri Modu
Lcd_Port(data >> 4);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
HAL_Delay(1);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
HAL_Delay(1);
Lcd_Port(data);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
HAL_Delay(1);
HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
HAL_Delay(2);
}
// Metin Yazma Fonksiyonu
void Lcd_String(char *str)
{
while(*str) Lcd_Char(*str++);
}
// İmleç Konumlandırma (0. veya 1. Satır)
void Lcd_Set_Cursor(uint8_t row, uint8_t col)
{
 uint8_t mask = (row == 0) ? 0x80 : 0xC0;
 Lcd_Cmd(mask | col);
}
// Ekran Temizleme
void Lcd_Clear(void)
{
Lcd_Cmd(0x01);
HAL_Delay(2);
}
// Başlangıç Ayarları (Initialization)
void Lcd_Init(void)
{
HAL_Delay(50); // Ekranın elektriksel olarak oturmasını bekle
Lcd_Cmd(0x02); // 4-Bit moda geçiş
Lcd_Cmd(0x28); // 2 Satır, 5x8 Font
Lcd_Cmd(0x0C); // Ekran Açık, İmleç Kapalı
Lcd_Cmd(0x06); // Yazdıkça imleci sağa kaydır
Lcd_Clear();
}
