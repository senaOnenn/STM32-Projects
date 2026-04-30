/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @brief   Yilan Oyunu — STM32F407 / ArmApp-18 Deney Seti
  *
  * DUZELTMELER (orijinal kod uzerinden):
  *   1. Oyun hizi 10 ms  →  40 ms  (foy gereksinimine uygun, oynanabilir)
  *   2. 'volatile' keyword'u ISR ile paylasilan degiskenlere eklendi
  *      (rx_buffer, yon_istek) — race-condition onlendi
  *   3. Yem yenme kontrolu eklendi (carpismaya dayali, tolerans = ADIM+1)
  *   4. Kendine carpma (self-collision) kontrolu eklendi
  *   5. Rastgele yem konumu: LCG (Linear Congruential Generator) ile uretilir
  *   6. Skor gosterimi: her yem = +10 puan, ekranin ust kosesinde gosterilir
  *   7. 'S' komutuyla oyun sifirlanip yeniden baslatilabilir
  *   8. goto yerine duzgun kontrol akisi
  *   9. Sinir cercevesi cizildi (oyun alani acik gorulur)
  *  10. Oyun bitis ekrani: skor gosterilir, 'S' ile tekrar baslanir
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* USER CODE BEGIN Includes */
#include "glcd.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef  hi2c1;
I2S_HandleTypeDef  hi2s3;
SPI_HandleTypeDef  hspi1;
TIM_HandleTypeDef  htim2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* --- ISR ile paylasilan degiskenler: MUTLAKA volatile olmali! --- */
volatile uint8_t rx_buffer  = 0;   /* UART'tan gelen ham byte            */
volatile char    yon_istek  = 'R'; /* ISR tarafindan yazilan yon istegi  */
volatile uint8_t restart_flag = 0; /* 'S' komutu geldiginde 1 olur       */

/* --- Oyun sabitleri --- */
#define MAX_UZUNLUK     100u
#define EKRAN_GEN       128u
#define EKRAN_YUK       64u
#define PARCA_BOY       3u      /* Yilan/yem kare boyutu (piksel)         */
#define ADIM            3u      /* Her hamlede kaydirma miktari (piksel)  */
#define OYUN_HIZI_MS    150u     /* Hamle periyodu ms — foy: 40 ms         */

/* Oyun alani ic sinirlar (1 piksel border + 1 boskluk) */
#define ALAN_SOL        2
#define ALAN_SAG        (EKRAN_GEN  - PARCA_BOY - 2)   /* 123 */
#define ALAN_UST        10      /* Ust kisimda skor icin yer birakildi   */
#define ALAN_ALT        (EKRAN_YUK  - PARCA_BOY - 2)   /* 59  */

/* --- Oyun durumu degiskenleri --- */
static int16_t  yilan_x[MAX_UZUNLUK];
static int16_t  yilan_y[MAX_UZUNLUK];
static uint8_t  uzunluk;
static char     yon;          /* Aktif yön (her frame'in basinda guncellenir) */
static int16_t  yem_x, yem_y;
static uint8_t  oyun_bitti;   /* 0=Oynuyor  1=GameOver(gosterilmedi)  2=Bekle */
static uint16_t skor;

/* Basit LCG rastgele sayi ureteci */
static uint32_t rand_seed = 12345UL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
static void     Oyun_Baslat(void);
static uint32_t Rand_Get(void);
static void     Yeni_Yem_Olustur(void);
static uint8_t  Yem_Yilana_Cakisiyor(int16_t x, int16_t y);
static void     Sinir_Ciz(void);
static void     Skor_Goster(void);
static void     Buzzer_Bip(uint32_t sure_ms);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ---------------------------------------------------------------
 * LCG: seed'i gunceller, 0..32767 araliginda sayi dondurur
 * --------------------------------------------------------------- */
static uint32_t Rand_Get(void)
{
    rand_seed = rand_seed * 1664525UL + 1013904223UL;
    return (rand_seed >> 16) & 0x7FFFu;
}

/* ---------------------------------------------------------------
 * Onerilen konum yilanin uzerinde mi?
 * --------------------------------------------------------------- */
static uint8_t Yem_Yilana_Cakisiyor(int16_t x, int16_t y)
{
    for (uint8_t i = 0; i < uzunluk; i++)
    {
        if (yilan_x[i] == x && yilan_y[i] == y) return 1u;
    }
    return 0u;
}

/* ---------------------------------------------------------------
 * Alan icinde, ADIM'a hizali, yilana carpismayan yem konumu uret
 * --------------------------------------------------------------- */
static void Yeni_Yem_Olustur(void)
{
    int16_t x, y;
    uint8_t deneme = 0;

    /* Grid genisligi ve yuksekligi (ADIM birimleriyle) */
    int16_t gx = (ALAN_SAG - ALAN_SOL) / ADIM;
    int16_t gy = (ALAN_ALT - ALAN_UST) / ADIM;

    do {
        x = ALAN_SOL + (int16_t)(Rand_Get() % (uint32_t)gx) * ADIM;
        y = ALAN_UST + (int16_t)(Rand_Get() % (uint32_t)gy) * ADIM;
        deneme++;
    } while (Yem_Yilana_Cakisiyor(x, y) && deneme < 50u);

    yem_x = x;
    yem_y = y;
}

/* ---------------------------------------------------------------
 * Oyun alani sinir cercevesini cizer
 * --------------------------------------------------------------- */
static void Sinir_Ciz(void)
{
    /* Yatay cizgiler */
    GLCD_DrawLine(0, ALAN_UST - 1, EKRAN_GEN - 1, ALAN_UST - 1, 1); /* Ust */
    GLCD_DrawLine(0, EKRAN_YUK - 1, EKRAN_GEN - 1, EKRAN_YUK - 1, 1); /* Alt */
    /* Dikey cizgiler */
    GLCD_DrawLine(0, ALAN_UST - 1, 0, EKRAN_YUK - 1, 1);              /* Sol */
    GLCD_DrawLine(EKRAN_GEN - 1, ALAN_UST - 1, EKRAN_GEN - 1, EKRAN_YUK - 1, 1); /* Sag */
}

/* ---------------------------------------------------------------
 * Skoru ekranin sol ust kosesine yazar
 * --------------------------------------------------------------- */
static void Skor_Goster(void)
{
    char buf[16];
    (void)sprintf(buf, "S:%u", (unsigned)skor);
    GLCD_WriteString(2, 1, buf);

    /* Uzunluk gostergesi (sag ust kose) */
    (void)sprintf(buf, "L:%u", (unsigned)uzunluk);
    GLCD_WriteString(90, 1, buf);
}

/* ---------------------------------------------------------------
 * Buzzer: sure_ms kadar oter, sonra kapanir
 * --------------------------------------------------------------- */
static void Buzzer_Bip(uint32_t sure_ms)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(sure_ms);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

/* ---------------------------------------------------------------
 * Oyunu (veya yeniden) baslat — tum state'i sifirla
 * --------------------------------------------------------------- */
static void Oyun_Baslat(void)
{
    uzunluk     = 4u;
    yon         = 'R';
    yon_istek   = 'R';
    oyun_bitti  = 0u;
    skor        = 0u;
    restart_flag = 0u;

    /* Yilani ekranin ortasina yatay diz */
    for (uint8_t i = 0; i < uzunluk; i++)
    {
        yilan_x[i] = 64  - (int16_t)(i * ADIM);
        yilan_y[i] = (ALAN_UST + ALAN_ALT) / 2;  /* Dikey orta */
    }

    /* Buzzer'i kapat */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

    Yeni_Yem_Olustur();
}

/* USER CODE END 0 */

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    /* MCU Baslangic Yapilandirmasi */
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_I2S3_Init();
    MX_SPI1_Init();
    MX_USB_HOST_Init();
    MX_TIM2_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */
    HAL_Delay(500);           /* Guç raylari kararlassin */

    /* Seed'i baslangic zamanina gore farklilas tir */
    rand_seed ^= HAL_GetTick();

    GLCD_Init();
    GLCD_Clear();
    GLCD_Update();

    Oyun_Baslat();

    /* Kesme tabanli UART dinlemesini baslat */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_buffer, 1);
    /* USER CODE END 2 */

    /* ====================================================
     * Ana Oyun Dongüsü
     * ==================================================== */
    uint32_t son_hareket = 0u;

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* Seed'i surekli besle (daha iyi rastgelelik) */
        rand_seed ^= HAL_GetTick();

        /* ---- Yeniden Baslama Istegi ('S' komutu) ---- */
        if (restart_flag)
        {
            Oyun_Baslat();
            son_hareket = HAL_GetTick();
        }

        /* ==================================================
         * DURUM 0 : OYUN OYNANIYOR
         * ================================================== */
        if (oyun_bitti == 0u)
        {
            if ((HAL_GetTick() - son_hareket) >= OYUN_HIZI_MS)
            {
                son_hareket = HAL_GetTick();

                /* 1. Yonu guncelle — ters yone donusu engelle ---------- */
                if      (yon_istek == 'U' && yon != 'D') yon = 'U';
                else if (yon_istek == 'D' && yon != 'U') yon = 'D';
                else if (yon_istek == 'L' && yon != 'R') yon = 'L';
                else if (yon_istek == 'R' && yon != 'L') yon = 'R';

                /* 2. Govde takibi: kuyruktan basa dogru kaydir ---------- */
                for (int8_t i = (int8_t)uzunluk - 1; i > 0; i--)
                {
                    yilan_x[i] = yilan_x[i - 1];
                    yilan_y[i] = yilan_y[i - 1];
                }

                /* 3. Bas hareketi ------------------------------------- */
                if (yon == 'U') yilan_y[0] -= ADIM;
                if (yon == 'D') yilan_y[0] += ADIM;
                if (yon == 'L') yilan_x[0] -= ADIM;
                if (yon == 'R') yilan_x[0] += ADIM;

                /* 4. Duvara carpma kontrolu --------------------------- */
                if (yilan_x[0] < ALAN_SOL  ||
                    yilan_x[0] > ALAN_SAG  ||
                    yilan_y[0] < ALAN_UST  ||
                    yilan_y[0] > ALAN_ALT)
                {
                    oyun_bitti = 1u;
                }

                /* 5. Kendine carpma kontrolu (govde = index 1'den itibaren) */
                if (oyun_bitti == 0u)
                {
                    for (uint8_t i = 1u; i < uzunluk; i++)
                    {
                        if (yilan_x[0] == yilan_x[i] &&
                            yilan_y[0] == yilan_y[i])
                        {
                            oyun_bitti = 1u;
                            break;
                        }
                    }
                }

                /* 6. Yem yenme kontrolu ------------------------------- */
                if (oyun_bitti == 0u)
                {
                    /* Toleransli cakisma: bas yeme ADIM+1 birim yakin mi? */
                    int16_t dx = yilan_x[0] - yem_x;
                    int16_t dy = yilan_y[0] - yem_y;
                    if (dx < 0) dx = -dx;
                    if (dy < 0) dy = -dy;

                    if (dx <= (int16_t)(ADIM + 1) && dy <= (int16_t)(ADIM + 1))
                    {
                        /* Yilani uzat */
                        if (uzunluk < MAX_UZUNLUK)
                        {
                            /* Yeni kuyruk parcasini son parcayla ayni konuma koy.
                               Bir sonraki hamlede govde takibi bunu dogru yere tasir. */
                            yilan_x[uzunluk] = yilan_x[uzunluk - 1];
                            yilan_y[uzunluk] = yilan_y[uzunluk - 1];
                            uzunluk++;
                        }
                        skor += 10u;

                        /* Kisa yem bip sesi */
                        Buzzer_Bip(30u);

                        /* Yeni yem olustur */
                        Yeni_Yem_Olustur();
                    }
                }

                /* 7. Cizim (Render) ----------------------------------- */
                GLCD_Clear();

                Skor_Goster();   /* Skor + uzunluk */
                Sinir_Ciz();     /* Oyun alani cercevesi */

                /* Yemi ciz (dolup 3x3 kare) */
                GLCD_DrawRect((uint8_t)yem_x, (uint8_t)yem_y,
                              PARCA_BOY, PARCA_BOY, 1);

                /* Yilani ciz */
                for (uint8_t i = 0u; i < uzunluk; i++)
                {
                    GLCD_DrawRect((uint8_t)yilan_x[i], (uint8_t)yilan_y[i],
                                  PARCA_BOY, PARCA_BOY, 1);
                }

                GLCD_Update();   /* Buffer'i ekrana yansit */
            }
        }

        /* ==================================================
         * DURUM 1 : GAME OVER — ilk gosterim
         * ================================================== */
        else if (oyun_bitti == 1u)
        {
            GLCD_Clear();

            GLCD_WriteString(28, 20, "GAME OVER!");

            /* Skoru goster */
            char buf[20];
            (void)sprintf(buf, "Skor: %u", (unsigned)skor);
            GLCD_WriteString(28, 34, buf);

            GLCD_WriteString(10, 48, "S: Tekrar Oyna");

            GLCD_Update();

            /* Uzun buzzer sesi */
            Buzzer_Bip(500u);

            oyun_bitti = 2u;   /* Bir kez gosterildi, bekleme moduna gec */
        }

        /* DURUM 2: Bekleme — restart_flag gelene kadar hic bir sey yapma */
        /* (restart_flag kontrolu dongünun basinda zaten var) */

        /* USER CODE END WHILE */

        MX_USB_HOST_Process();

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/* ============================================================
 * KESME SERVIS RUTINI (ISR)
 * ============================================================ */

/* USER CODE BEGIN 4 */
/**
 * @brief  UART alim tamamlama geri cagrimi.
 *         Bu fonksiyon ISR baglaminda calisir; sadece volatile
 *         degiskenlere yazmali, uzun islemlerden kacınmali.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        switch (rx_buffer)
        {
            case 'U': if (yon != 'D') yon_istek = 'U'; break;
            case 'D': if (yon != 'U') yon_istek = 'D'; break;
            case 'L': if (yon != 'R') yon_istek = 'L'; break;
            case 'R': if (yon != 'L') yon_istek = 'R'; break;
            case 'S': restart_flag = 1u;                break;
            default:  break;
        }

        /* Bir sonraki komut icin kesmeyi yeniden kur */
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_buffer, 1);
    }
}
/* USER CODE END 4 */

/* ============================================================
 * DONANIM BASLANGIC FONKSIYONLARI (CubeMX uretimi, degistirilmedi)
 * ============================================================ */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance              = I2C1;
    hi2c1.Init.ClockSpeed       = 100000;
    hi2c1.Init.DutyCycle        = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_I2S3_Init(void)
{
    hi2s3.Instance            = SPI3;
    hi2s3.Init.Mode           = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard       = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat     = I2S_DATAFORMAT_16B;
    hi2s3.Init.MCLKOutput     = I2S_MCLKOUTPUT_ENABLE;
    hi2s3.Init.AudioFreq      = I2S_AUDIOFREQ_96K;
    hi2s3.Init.CPOL           = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource    = I2S_CLOCK_PLL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    if (HAL_I2S_Init(&hi2s3) != HAL_OK) Error_Handler();
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 16000 - 1;   /* 168 MHz / 16000 = 10500 Hz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 100 - 1;      /* 10500 / 100 = 105 Hz ~ 9.5 ms */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Cikis pinleri varsayilan duzey */
    HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
                           | GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin
                           | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
                           | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  /* Buzzer kapali */

    /* CS_I2C_SPI */
    GPIO_InitStruct.Pin   = CS_I2C_SPI_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

    /* OTG_FS_PowerSwitchOn */
    GPIO_InitStruct.Pin   = OTG_FS_PowerSwitchOn_Pin;
    HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /* PDM_OUT */
    GPIO_InitStruct.Pin       = PDM_OUT_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

    /* B1 (user button) */
    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /* GLCD kontrol pinleri: PB0-PB5 */
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
                          | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CLK_IN */
    GPIO_InitStruct.Pin       = CLK_IN_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

    /* GLCD veri pinleri + LED'ler: PD0-PD7 */
    GPIO_InitStruct.Pin   = LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin
                          | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
                          | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Buzzer: PA8 */
    GPIO_InitStruct.Pin   = GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* MEMS_INT2 */
    GPIO_InitStruct.Pin  = MEMS_INT2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { /* Sonsuz dongu */ }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif
