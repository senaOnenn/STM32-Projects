#include "lcd.h"
// Yardımcı Fonksiyon: 4-Bit veriyi pinlere dağıtır
void Lcd_Port(uint8_t data) {
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
void Lcd_Cmd(uint8_t cmd) {
 HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_RESET); // Komut Modu

 Lcd_Port(cmd >> 4); // Üst 4 bit
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
 HAL_Delay(2); // 1 yerine 2 yaptık (Garanti olsun)
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
 HAL_Delay(2);

 Lcd_Port(cmd); // Alt 4 bit
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
 HAL_Delay(2);
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
 HAL_Delay(5); // Komutun işlenmesi için uzun süre tanı
}
// LCD'ye Veri Yazma (RS=1)
void Lcd_Char(char data) {
 HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_SET); // Veri Modu

 Lcd_Port(data >> 4);
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
 HAL_Delay(2);
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
 HAL_Delay(2);

 Lcd_Port(data);
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
 HAL_Delay(2);
 HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
 HAL_Delay(2);
}
// Metin Yazma Fonksiyonu
void Lcd_String(char *str) {
 while(*str) Lcd_Char(*str++);
}
// İmleç Konumlandırma
void Lcd_Set_Cursor(uint8_t row, uint8_t col) {
 uint8_t mask = (row == 0) ? 0x80 : 0xC0;
 Lcd_Cmd(mask | col);
}
// Ekran Temizleme
void Lcd_Clear(void) {
 Lcd_Cmd(0x01);
 HAL_Delay(2);
}
// Başlangıç Ayarları (Initialization)
void Lcd_Init(void) {
 HAL_Delay(50); // Ekranın güç almasını bekle
 Lcd_Cmd(0x02); // 4-Bit moda geçiş
 Lcd_Cmd(0x28); // 2 Satır, 5x8 Font
 Lcd_Cmd(0x0C); // Ekran Açık, İmleç Kapalı
 Lcd_Cmd(0x06); // Yazdıkça sağa kaydır
 Lcd_Clear();
}
