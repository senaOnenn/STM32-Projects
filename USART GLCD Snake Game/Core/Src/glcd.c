/*
 * glcd.c  —  KS0108 tabanli 128x64 Grafik LCD surucusu
 * HAL_Delay kullanilan stabil versiyon
 */

#include "glcd.h"
#include "font5x8.h"
#include <string.h>
#include <stdio.h>

static uint8_t glcd_buffer[1024];

/* Mikrosaniye beklemesi — HAL_Delay(1) = 1ms cok uzun,
 * KS0108 EN pulse icin sadece ~450 ns yeterli.
 * 168 MHz STM32F407: 1 NOP ~ 6 ns → 200 NOP ~ 1.2 us (guvenli margin) */
static inline void delay_us(void)
{
    for (volatile uint16_t i = 0; i < 200u; i++) { __NOP(); }
}

static void GLCD_Enable(void)
{
    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_EN_PIN, GPIO_PIN_SET);
    delay_us();   /* ~1.2 us  (KS0108 min: 450 ns) */
    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_EN_PIN, GPIO_PIN_RESET);
    delay_us();   /* cycle time icin ek bekleme     */
}

static void GLCD_Write(uint8_t data, uint8_t isData, uint8_t chip)
{
    if (chip == 1)
    {
        HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_CS1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_CS2_PIN, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_CS1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_CS2_PIN, GPIO_PIN_SET);
    }

    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_RS_PIN,
                      isData ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_RW_PIN, GPIO_PIN_RESET);

    GLCD_DATA_PRT->ODR = (GLCD_DATA_PRT->ODR & 0xFF00U) | (uint32_t)data;

    GLCD_Enable();
}

void GLCD_Init(void)
{
    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GLCD_CTRL_PRT, GLCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    GLCD_Write(0x3F, 0, 1);
    GLCD_Write(0x3F, 0, 2);

    GLCD_Write(0xB8, 0, 1);
    GLCD_Write(0xB8, 0, 2);
    GLCD_Write(0x40, 0, 1);
    GLCD_Write(0x40, 0, 2);

    GLCD_Clear();
    GLCD_Update();
}

void GLCD_Clear(void)
{
    memset(glcd_buffer, 0x00, sizeof(glcd_buffer));
}

void GLCD_Update(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        GLCD_Write(0xB8 | page, 0, 1);
        GLCD_Write(0x40,        0, 1);
        for (uint8_t x = 0; x < 64; x++)
            GLCD_Write(glcd_buffer[(uint16_t)page * 128u + x], 1, 1);

        GLCD_Write(0xB8 | page, 0, 2);
        GLCD_Write(0x40,        0, 2);
        for (uint8_t x = 64; x < 128; x++)
            GLCD_Write(glcd_buffer[(uint16_t)page * 128u + x], 1, 2);
    }
}

void GLCD_SetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= 128u || y >= 64u) return;
    uint16_t index = (uint16_t)(y / 8u) * 128u + x;
    uint8_t  bit   = (uint8_t)(1u << (y % 8u));
    if (color) glcd_buffer[index] |=  bit;
    else       glcd_buffer[index] &= ~bit;
}

void GLCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t i = x; i < (uint8_t)(x + w); i++)
        for (uint8_t j = y; j < (uint8_t)(y + h); j++)
            GLCD_SetPixel(i, j, color);
}

void GLCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    int16_t dx  = (int16_t)x1 - x0;
    int16_t dy  = (int16_t)y1 - y0;
    int16_t adx = (dx < 0) ? -dx : dx;
    int16_t ady = (dy < 0) ? -dy : dy;
    int16_t sx  = (dx >= 0) ? 1 : -1;
    int16_t sy  = (dy >= 0) ? 1 : -1;
    int16_t err = adx - ady;
    while (1)
    {
        GLCD_SetPixel((uint8_t)x0, (uint8_t)y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -ady) { err -= ady; x0 = (uint8_t)(x0 + sx); }
        if (e2 <  adx) { err += adx; y0 = (uint8_t)(y0 + sy); }
    }
}

void GLCD_WriteString(uint8_t x, uint8_t y, const char *str)
{
    while (*str)
    {
        char c = *str;
        if (c >= 32 && c <= 126)
        {
            uint16_t base = (uint16_t)(c - 32) * 5u;
            for (uint8_t col = 0; col < 5u; col++)
            {
                uint8_t line = font5x8[base + col];
                for (uint8_t row = 0; row < 8u; row++)
                    GLCD_SetPixel((uint8_t)(x + col), (uint8_t)(y + row),
                                  (line >> row) & 0x01u);
            }
        }
        x = (uint8_t)(x + 6u);
        str++;
        if (x > 122u) break;
    }
}

void GLCD_WriteInt(uint8_t x, uint8_t y, int32_t val)
{
    char buf[12];
    (void)sprintf(buf, "%ld", (long)val);
    GLCD_WriteString(x, y, buf);
}
