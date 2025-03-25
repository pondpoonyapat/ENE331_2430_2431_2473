/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "main.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

unsigned int SW_State;
uint8_t currentState = 0; // เก็บสถานะปัจจุบัน (0..3)
uint8_t lastButton = 1; // ด้าน Hardware: PA0 Pull-up → ไม่กด = 1, กด = 0
uint8_t button; // เก็บค่าสถานะปุ่มรอบล่าสุด
uint8_t buttonCheck; //ไว้เช็คอีกรอบกัน Switch Debounce

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);

void GPIO_Config(void) {
    // เปิด Clock GPIOA และ GPIOB
    RCC->AHB1ENR |= (0b1 << 0); // (bit0) => GPIOA
    RCC->AHB1ENR |= (0b1 << 1); // (bit1) => GPIOB

    // ตั้งค่า PA0 เป็น Input
    // MODER มี 2 bits ต่อ 1 pin: 00=Input, 01=Output, 10=AF, 11=Analog
    // พิน 0 => bit[1:0]
    GPIOA->MODER |= (0b00 << (0* 2)); // เคลียร์ 2 bits => 00 => Input

    // ตั้งค่า PA1, PA2, PA3, PA6 เป็น Output
    // พิน 1 => bit[3:2], พิน 2 => bit[5:4], พิน 3 => bit[7:6], พิน 6 => bit[13:12]
    // ให้เป็น 01 => Output
    GPIOA->MODER |= (0b01 << (1* 2)); //LED 1
    GPIOA->MODER |= (0b01 << (2* 2)); //LED 2
    GPIOA->MODER |= (0b01 << (3* 2)); //LED 3
    GPIOA->MODER |= (0b01 << (6* 2)); //Blink

    // ตั้งค่า PB10 เป็น Output
    // พิน 10 => bit[21:20], 01 => Output
    GPIOB->MODER |= (0b01 << (10* 2)); //LED 3

    // ตั้งค่า Output Speed ให้เป็น High Speed
    GPIOA->OSPEEDR &= ~(0x00);
    GPIOB->OSPEEDR &= ~(0x00);

    // ตั้งค่า PA0 ให้เป็น pull-up ส่วนที่เหลือให้ disable
    GPIOA->PUPDR |= (0b01 << (0* 2));
    GPIOA->PUPDR |= (0b00 << (1* 2));
    GPIOA->PUPDR |= (0b00 << (2* 2));
    GPIOA->PUPDR |= (0b00 << (3* 2));
    GPIOA->PUPDR |= (0b00 << (6* 2));
    GPIOB->PUPDR |= (0b00 << (10* 2));
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
	// Timer2 เกิด interrupt ทุก 20us
	// Toggle PB10 เพื่อนำสัญญาณไปดูที่ Oscilloscope
    GPIOB->ODR ^= (1 << 10);
  }
  if (htim->Instance == TIM3)
  {
	// Toggle LED ที่ PA6 โดยตรง (Register)
	GPIOA->ODR ^= (1 << 6);
  }
}

unsigned char Read_PA0(void) {
    // อ่านค่าใน Input Data Register ของ GPIOA ที่ bit0
    return (GPIOA->IDR & (1 << 0)) ? 1 : 0;
	}


int main(void)
{

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  GPIO_Config();

 // เปิดใช้งาน Timer Interrupt
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);

  while (1)
  {
	  // 1) อ่านปุ่ม
	  button = Read_PA0(); // ถ้าดึง pull-up => ปกติ = 1, กด = 0
	  // 2) ตรวจจับขอบตก (falling edge) จาก 1 → 0
	  if (button == 0 && lastButton == 1)
	  {
		  HAL_Delay(20);
		  buttonCheck = Read_PA0();
		  if (buttonCheck == 0){
			  // เปลี่ยน state
			  currentState++;
			  if (currentState > 3)
			  {
				  currentState = 0;
			  }
		  }
	  }
	  lastButton = button; // เก็บสถานะปุ่มรอบนี้เป็น "อดีต" ไว้ใช้เทียบในรอบหน้า

	  // 3) สั่ง LED ตาม State ปัจจุบัน
	  switch (currentState)
	  {
	      case 0:
	          // State 0 => PA1=1, PA2=0, PA3=0
	          GPIOA->ODR |=  (1 << 1);       // เปิด PA1
	          GPIOA->ODR &= ~(1 << 2);       // ปิด PA2
	          GPIOA->ODR &= ~(1 << 3);       // ปิด PA3
	          break;

	      case 1:
	          // State 1 => PA1=0, PA2=1, PA3=0
	          GPIOA->ODR &= ~(1 << 1);
	          GPIOA->ODR |=  (1 << 2);
	          GPIOA->ODR &= ~(1 << 3);
	          break;

	      case 2:
	          // State 2 => PA1=0, PA2=0, PA3=1
	          GPIOA->ODR &= ~(1 << 1);
	          GPIOA->ODR &= ~(1 << 2);
	          GPIOA->ODR |=  (1 << 3);
	          break;

	      case 3:
	          // State 3 => PA1=1, PA2=1, PA3=1
	          GPIOA->ODR |=  (1 << 1);
	          GPIOA->ODR |=  (1 << 2);
	          GPIOA->ODR |=  (1 << 3);
	          break;

	      default:
	          // ป้องกัน error case (ถ้าเกิน 3) ก็กลับไป state 0
	          currentState = 0;
	          break;
	  }
  }

}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 719;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7853;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
