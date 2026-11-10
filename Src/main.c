/**
  ******************************************************************************
  * @file    DAC/DAC_SignalsGeneration/Src/main.c
  * @author  MCD Application Team
  * @brief   This example provides a short description of how to use the DAC
  *          peripheral to generate several signals.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>  // Adicione esta linha

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup DAC_SignalsGeneration
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    float freq_inicial;
    float freq_final;
    uint32_t duracao_ms;
    uint16_t prescaler;
    float clock_efetivo;
} SweepStage;


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef    DacHandle;
static DAC_ChannelConfTypeDef sConfig;


/* ========================================================================= */
/* COLOQUE A TABELA EXATAMENTE AQUI (COMO UMA VARIÁVEL GLOBAL)               */
/* ========================================================================= */
/* Tabela na seção .RAM_D1 (AXI SRAM) para o DMA ler sem travar o barramento */

/* Tabela Seno de 8 bits - 128 pontos (Atenuada para não saturar o topo do Buffer) */
const uint8_t dynamic_sine_8bit[128] = {
    128, 133, 138, 143, 148, 153, 158, 163, 168, 173, 178, 183, 187, 192, 196, 200,
    204, 207, 211, 214, 217, 220, 222, 224, 226, 227, 229, 229, 230, 230, 230, 230,
    230, 230, 230, 229, 229, 227, 226, 224, 222, 220, 217, 214, 211, 207, 204, 200,
    196, 192, 187, 183, 178, 173, 168, 163, 158, 153, 148, 143, 138, 133, 128, 123,
    118, 113, 108, 103,  98,  93,  88,  83,  78,  73,  69,  64,  60,  56,  52,  49,
     45,  42,  39,  36,  33,  30,  28,  26,  24,  23,  21,  21,  20,  20,  20,  20,
     20,  20,  20,  21,  21,  23,  24,  26,  28,  30,  33,  36,  39,  42,  45,  49,
     52,  56,  60,  64,  69,  73,  78,  83,  88,  93,  98, 103, 108, 113, 118, 123
};


/* Variáveis Globais de Controle do Sweep */
uint8_t estagio_atual = 0;
uint32_t tempo_estagio_ms = 0;

/* Definição exata da sua tabela de Sweeps (5 estágios sequenciais) */
const SweepStage tabela_sweeps[5] = {
    {5.0f,   22.5f,  360000, 9, 20000000.0f},  /* Estágio 1: 5 a 22.5 Hz em 6 min (PSC=9) */
    {22.5f,  110.0f, 300000, 0, 200000000.0f}, /* Estágio 2: 22.5 a 110 Hz em 5 min (PSC=0) */
    {110.0f, 500.0f, 240000, 0, 200000000.0f}, /* Estágio 3: 110 a 500 Hz em 4 min (PSC=0) */
    {500.0f, 900.0f, 180000, 0, 200000000.0f}, /* Estágio 4: 500 a 900 Hz em 3 min (PSC=0) */
    {900.0f, 2000.0f,120000, 0, 200000000.0f}  /* Estágio 5: 900 a 2000 Hz em 2 min (PSC=0) */
};



/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void DAC_Ch1_SineConfig(void);
static void TIM6_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);
static void Processar_Sweep_Senoide(void);
/* Private functions ---------------------------------------------------------*/



/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{


  
  /* Configure the MPU attributes */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();
  
   /* Configure LED2 */
  BSP_LED_Init(LED2);


  /*##-1- Configure the DAC peripheral #######################################*/
  DacHandle.Instance = DACx;

  /*##-2- Configure the TIM peripheral #######################################*/
  TIM6_Config();

  /* LIGA A SENOIDE DIRETAMENTE NA INICIALIZAÇÃO */
    DAC_Ch1_SineConfig();

  /* Infinite loop */
  /* Infinite loop */
  while (1)
  {
	  /* Processa continuamente a rampa de frequência da senoide */
	     Processar_Sweep_Senoide();
  }

}

/**
  * @brief  Gerenciador do Sweep Linear Automático em Loop Infinito
  * @retval None
  */
static void Processar_Sweep_Senoide(void)
{
  static uint32_t tempo_anterior = 0;
  uint32_t tempo_atual = HAL_GetTick();

  /* Executa o ajuste estritamente a cada 100 milissegundos */
  if ((tempo_atual - tempo_anterior) >= 100)
  {
    tempo_anterior = tempo_atual;

    /* Se ainda estamos dentro dos 5 estágios válidos */
    if (estagio_atual < 5)
    {
      const SweepStage *stage = &tabela_sweeps[estagio_atual];
      tempo_estagio_ms += 100;

      /* Verifica se o tempo do estágio atual chegou ao fim */
      if (tempo_estagio_ms >= stage->duracao_ms)
      {
        tempo_estagio_ms = 0;
        estagio_atual++; /* Pula para o próximo estágio */

        /* Se mudou de estágio e ainda é válido, aplica o novo Prescaler */
        if (estagio_atual < 5)
        {
          TIM6->PSC = tabela_sweeps[estagio_atual].prescaler;
          TIM6->EGR = TIM_EGR_UG; /* Força o hardware a atualizar o PSC imediatamente */
        }
        return;
      }

      /* Cálculo da rampa linear de Frequência para o milissegundo atual */
      float fracao_tempo = (float)tempo_estagio_ms / (float)stage->duracao_ms;
      float delta_freq = stage->freq_final - stage->freq_inicial;
      float freq_atual = stage->freq_inicial + (delta_freq * fracao_tempo);

      /* Calcula o novo valor de ARR baseado no clock do estágio e nos 128 pontos */
      uint32_t novo_arr = (uint32_t)(stage->clock_efetivo / (freq_atual * 128.0f)) - 1;

      /* Proteção contra estouro de registro (16 bits) */
      if (novo_arr > 65535) novo_arr = 65535;

      /* Atualiza o hardware do Timer 6 */
      TIM6->ARR = novo_arr;
    }
    else
    {
      /* REINICIALIZAÇÃO DO LOOP: Ao fim dos 20 min, volta para o Estágio 1 (5 Hz) */
      estagio_atual = 0;
      tempo_estagio_ms = 0;

      TIM6->PSC = tabela_sweeps[0].prescaler; /* PSC = 9 */
      TIM6->ARR = 31249;                      /* ARR para 5 Hz */
      TIM6->CNT = 0;
      TIM6->EGR = TIM_EGR_UG;                 /* Força atualização dos registradores */
    }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE BYPASS)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 4
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

}
/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED2 on */
  BSP_LED_On(LED2);

  while(1)
  {
  }
}



/**
  * @brief  DAC Channel1 Sine Configuration
  * @param  None
  * @retval None
  */
static void DAC_Ch1_SineConfig(void)
{
  /* Reseta a máquina de estados para iniciar do início absoluto (5 Hz) */
  estagio_atual = 0;
  tempo_estagio_ms = 0;

  /* Força as condições físicas de partida no Timer 6 */
  TIM6->PSC = 9;
  TIM6->ARR = 31249;
  TIM6->CNT = 0;
  TIM6->EGR = TIM_EGR_UG;

  if (HAL_DAC_Init(&DacHandle) != HAL_OK) { Error_Handler(); }

  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

  if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DACx_CHANNEL) != HAL_OK) { Error_Handler(); }

  /* Limpa o cache de dados garantindo que o DMA leia o valor real atualizado */
  uint32_t *pCacheAddr = (uint32_t *)((uint32_t)dynamic_sine_8bit & ~(0x1FU));
  SCB_CleanDCache_by_Addr(pCacheAddr, sizeof(dynamic_sine_8bit));

  /* Dispara o DMA nativo em modo 8 bits */
  if (HAL_DAC_Start_DMA(&DacHandle, DACx_CHANNEL, (uint32_t *)dynamic_sine_8bit, 128, DAC_ALIGN_8B_R) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief  TIM6 Configuration
  * @note   TIM6 configuration is based on APB1 frequency
  * @note   TIM6 Update event occurs each TIM6CLK/256
  * @param  None
  * @retval None
  */
void TIM6_Config(void)
{
  static TIM_HandleTypeDef  htim;
  TIM_MasterConfigTypeDef sMasterConfig;

  memset(&htim, 0, sizeof(htim));

  htim.Instance = TIM6;
  htim.Init.Period            = 31249;
  htim.Init.Prescaler         = 9;
  htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim.Init.RepetitionCounter = 0;

  HAL_TIM_Base_Init(&htim);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

  HAL_TIM_Base_Start(&htim);
}

/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif


/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}
/**
  * @}
  */

/**
  * @}
  */

