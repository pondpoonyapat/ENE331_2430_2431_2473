#include "main.h"

TIM_HandleTypeDef htim3;

int count_timer = 0;
int count_state = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);

void GPIOA_Config(void) {
    // Enable GPIOA Clock
    RCC -> AHB1ENR |= (0x01 << 0);

    // Set PA1 - PA3, PA6 as output
    GPIOA -> MODER |= (0x00 << (0* 2));
    GPIOA -> MODER |= (0x01 << (1* 2));   // LED 1
    GPIOA -> MODER |= (0x01 << (2* 2));   // LED 2
    GPIOA -> MODER |= (0x01 << (3* 2));   // LED 3
    GPIOA -> MODER |= (0x01 << (6* 2));   // LED 6 Blinking

    // Set output type
    GPIOA -> OTYPER &= (0x00);  // ALL GPIO (PA) output push-pull

    // Set GPIO Speed
    GPIOA -> OSPEEDR &= ~(0x00);  // HIGH speed for PA6

    // PA0 pull-up
    GPIOA -> PUPDR |= (0x01 << (0 * 2));

    // Disable PA1 - PA3, PA6 pull-up
    GPIOA -> PUPDR |= (0x00 << (1 * 2));
    GPIOA -> PUPDR |= (0x00 << (2 * 2));
    GPIOA -> PUPDR |= (0x00 << (3 * 2));
    GPIOA -> PUPDR |= (0x00 << (6 * 2));
}

void GPIOB_Config(void) {
	// Enable B Clock
	RCC -> AHB1ENR |= (0x01 << 1);

	// Config PB10 as output
	GPIOB -> MODER |= (0x01 << (10 * 2));

	// Set output type
	// ALL GPIO (PB) is  Output push-pull
	GPIOB-> OTYPER &= (0x00);

	// ALL GPIO (PBA) Speed is High speed Output
	GPIOB -> OSPEEDR &= ~(0x00);

	// Disable PB10 pull-up
	GPIOB -> PUPDR |= (0x00 << (10 * 2));
}

unsigned char read_PA0(void) {
	return (GPIOA -> IDR & (0x01 << 0)) ? 1 : 0;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
	if(htim -> Instance == TIM3) {
		count_timer++;
		GPIOB -> ODR ^= (1 << 10);
	}
}

int main(void)
{

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM3_Init();
  GPIOA_Config();
  GPIOB_Config();
  HAL_TIM_Base_Start_IT(&htim3);

  uint8_t button_state = 0;
  uint8_t last_unbutton_state = 0;

  while (1)
  {
	  uint8_t button_state = read_PA0();

	  if(button_state == 0 && last_unbutton_state == 1) {
		  count_state ++;
		  if (count_state >= 4) {
			  count_state = 0;
		  }
	  }

	  if(count_state == 0){
		  GPIOA -> ODR |= (0x01 << 1);
		  GPIOA -> ODR &= ~(0x01 << 2);
		  GPIOA -> ODR &= ~(0x01 << 3);
	  }
	  else if (count_state == 1){
		  GPIOA -> ODR &= ~(0x01 << 1);
		  GPIOA -> ODR |= (0x01 << 2);
		  GPIOA -> ODR &= ~(0x01 << 3);
	  }
	  else if (count_state == 2){
		  GPIOA -> ODR &= ~(0x01 << 1);
		  GPIOA -> ODR &= ~(0x01 << 2);
		  GPIOA -> ODR |= (0x01 << 3);
	  }

	  if (count_timer >= 120) {
		  GPIOA -> ODR ^= (1 << 6);
		  count_timer = 0;
	  }

	  last_unbutton_state = button_state;
  }

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
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

static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 1440-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1;
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
