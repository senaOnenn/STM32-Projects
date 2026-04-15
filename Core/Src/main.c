/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ARMapp-18 STM32F407 DC Motor PWM & Encoder Kontrol
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <math.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim9;

/* ÇÖZÜM 1: 'volatile' eklendi! Kesme içinde güncellenen değişkenler her zaman volatile olmalıdır. */
volatile float enc_value = 0.0f;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM9_Init(void);

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM9_Init();

  /* PWM Sinyalini Baslat ve Baslangicta Sifirda Tut */
  HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
  __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);

  /* Infinite loop */
  while (1)
  {
      // 1. LED Kademesi Hesaplama (Her 12.5 birim 1 LED yakar)
      uint8_t num_leds = (uint8_t)(fabs(enc_value) / 12.5f);
      if (num_leds > 8) num_leds = 8;

      // LED0'dan (PE0) baslayip cogalan maske (Orn: 3 LED icin 0b00000111)
      uint32_t led_mask = (1 << num_leds) - 1;

      // ODR durumunu al, ilk 8 biti (PE0-PE7) temizle
      uint32_t current_odr = GPIOE->ODR;
      current_odr &= ~0x00FF;

      // Istenen saf LED seviyesini uygula
      current_odr |= led_mask;

      // 2. SW0 (Slide Switch) ve Motor Kontrolu
      if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_13) == GPIO_PIN_SET)
      {
          // SW0 Aktif - Motor Calisacak
          uint32_t duty_cycle = (uint32_t)fabs(enc_value);
          __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, duty_cycle);

          if (enc_value > 0.0f) // Saat Yönü (CW) - Pozitif Değerler
          {
              current_odr |= GPIO_PIN_4;  // IN1 = 1
              current_odr &= ~GPIO_PIN_6; // IN2 = 0
          }
          else if (enc_value < 0.0f) // Saat Yönünün Tersi (CCW) - Negatif Değerler
          {
              current_odr &= ~GPIO_PIN_4; // IN1 = 0
              current_odr |= GPIO_PIN_6;  // IN2 = 1
          }
          else // Hız Sifir
          {
              current_odr &= ~GPIO_PIN_4;
              current_odr &= ~GPIO_PIN_6;
          }
      }
      else
      {
          // SW0 Pasif - Motor Durmali
          __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);
          current_odr &= ~GPIO_PIN_4;
          current_odr &= ~GPIO_PIN_6;
      }

      // 3. Tum GPIOE guncellemelerini tek seferde yaz (Motor pinleri LED maskesini ezecektir, bu donanimsal olarak normaldir)
      GPIOE->ODR = current_odr;

      HAL_Delay(10); // Döngü stabilitesi
  }
}

/**
  * @brief EXTI Hat Kesmesi Geri Cagirma Fonksiyonu (Encoder CLK Tetiklemesi)
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_time = HAL_GetTick();
    static uint32_t last_interrupt_time = 0;

    if (GPIO_Pin == GPIO_PIN_13) // Encoder CLK Pini (PB13)
    {
        // ÇÖZÜM 2: Gecikme 10ms'ye çıkarıldı. Mekanik encoder'lar için 2ms çok kısadır ve birden fazla tetikleme yapar.
        if (current_time - last_interrupt_time > 10)
        {
            // DT Pini (PB12) okunarak yön tayin edilir
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_SET)
            {
                enc_value += 12.5f; // Saat yönünde arttır
            }
            else
            {
                enc_value -= 12.5f; // Saatin tersi yönünde azalt
            }

            // Encoder sinirlari [-100.0, 100.0] olarak tutulur
            if (enc_value > 100.0f) enc_value = 100.0f;
            if (enc_value < -100.0f) enc_value = -100.0f;

            last_interrupt_time = current_time;
        }
    }
}

/**
  * @brief System Clock Configuration (168 MHz)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM9 Initialization Function
  */
static void MX_TIM9_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  __HAL_RCC_TIM9_CLK_ENABLE();

  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 335;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 99;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                          GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /* Configure PE0-PE4, PE6, PE7 as Output (LED ve Motor) */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                          GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Configure PE5 as TIM9 CH1 Alternate Function (PWM Çıkışı) */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Configure PE13 as Input (Slide Switch 0) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Configure PB12 as Input (Encoder DT Data Sinyali) */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure PB13 as EXTI (Encoder CLK Saat Sinyali - Düşen Kenar) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
