#ifndef GLCD_H
#define GLCD_H

#include "stm32f4xx_hal.h"

/* ---------------------------------------------------------------
 * Donanim Pin Tanimlari  (ArmApp-18 deney seti semasi)
 * --------------------------------------------------------------- */
#define GLCD_CS1_PIN   GPIO_PIN_0   /* Sol chip secici  - PORTB */
#define GLCD_CS2_PIN   GPIO_PIN_1   /* Sag chip secici  - PORTB */
#define GLCD_RS_PIN    GPIO_PIN_2   /* Veri/Komut sec.  - PORTB */
#define GLCD_RW_PIN    GPIO_PIN_3   /* Oku/Yaz secici   - PORTB */
#define GLCD_EN_PIN    GPIO_PIN_4   /* Enable (tetik)   - PORTB */
#define GLCD_RST_PIN   GPIO_PIN_5   /* Reset            - PORTB */

#define GLCD_CTRL_PRT  GPIOB        /* Kontrol port     */
#define GLCD_DATA_PRT  GPIOD        /* 8-bit veri yolu  PD0-PD7 */

/* ---------------------------------------------------------------
 * Public fonksiyon prototipleri
 * --------------------------------------------------------------- */
void GLCD_Init(void);
void GLCD_Clear(void);
void GLCD_Update(void);
void GLCD_SetPixel(uint8_t x, uint8_t y, uint8_t color);
void GLCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void GLCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void GLCD_WriteString(uint8_t x, uint8_t y, const char *str);
void GLCD_WriteInt(uint8_t x, uint8_t y, int32_t val);

#endif /* GLCD_H */
