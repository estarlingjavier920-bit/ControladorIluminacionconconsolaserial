
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    MODE_MANUAL25,
    MODE_MANUAL40,
    MODE_MANUAL60,
    MODE_MANUAL80,
    MODE_AUTO,
    MODE_PAUSE
} SystemState_t;

typedef enum
{
    BTN_IDLE,
    BTN_PRESSED
} ButtonState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* Variables de la rampa automatica */
static uint8_t autoDuty = 10U;
static int8_t rampDirection = 1;
static uint32_t lastRampTick = 0U;

/* Contador de milisegundos */
volatile uint32_t g_msTicks = 0U;

/*Almacena caracteres desde la terminal en tiempo real*/

static uint8_t rxByte = 0U;

/* Bandera de acciones del pulsador:
   0 = ninguna
   1 = pulsacion corta
   2 = pulsacion larga
*/
volatile uint8_t buttonActionFlag = 0U;
volatile uint8_t g_extiTriggered = 0U;

/* Estados del sistema */
SystemState_t currentSystemState = MODE_MANUAL25;
SystemState_t stateBeforePause = MODE_MANUAL25;

/* Variables del pulsador */
static ButtonState_t btnState = BTN_IDLE;
static GPIO_PinState candidateButtonState = GPIO_PIN_SET;
static GPIO_PinState stableButtonState = GPIO_PIN_SET;

static uint32_t debounceStart = 0U;
static uint32_t btnPressTime = 0U;
static uint8_t longPressReported = 0U;
/* TELEMETRIA  Variable recuerda cuándo se envió el último mensaje.*/
static uint32_t lastTelemetryTick = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */

void Set_PWM_Duty(uint32_t channel, uint8_t duty);
void Button_Process(uint32_t currentMillis);
void App_ProcessState(uint32_t now);
void Error_Handler(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =========================================================
   INTERRUPCION DEL TEMPORIZADOR TIM4
   Debe producir una interrupcion cada 1 ms
   ========================================================= */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        g_msTicks++;
    }
}

/* =========================================================
   CONTROL DEL CICLO DE TRABAJO PWM
   ========================================================= */

void Set_PWM_Duty(uint32_t channel, uint8_t duty)
{
    if (duty > 100U)
    {
        return;
    }

    uint32_t period =
        __HAL_TIM_GET_AUTORELOAD(&htim2) + 1U;

    uint32_t pulse =
        (period * (uint32_t)duty) / 100U;

    __HAL_TIM_SET_COMPARE(&htim2, channel, pulse);
}

/* =========================================================
   INTERRUPCION EXTI DEL PULSADOR PC13
   ========================================================= */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        g_extiTriggered = 1U;
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        switch (rxByte)
        {
            case '1':
                currentSystemState = MODE_MANUAL25;
                break;

            case '2':
                currentSystemState = MODE_MANUAL40;
                break;

            case '3':
                currentSystemState = MODE_MANUAL60;
                break;

            case '4':
                currentSystemState = MODE_MANUAL80;
                break;

            case 'A':
            case 'a':
                currentSystemState = MODE_AUTO;
                break;

            case 'P':
            case 'p':
                if (currentSystemState != MODE_PAUSE)
                {
                    stateBeforePause = currentSystemState;
                    currentSystemState = MODE_PAUSE;
                }
                else
                {
                    currentSystemState = stateBeforePause;
                }
                break;

            default:
                /* Ignorar caracteres no reconocidos */
                break;
        }

        /* Preparar la recepcion del siguiente caracter */
        HAL_UART_Receive_IT(&huart2, &rxByte, 1);
    }
}

/* =========================================================
   PROCESAMIENTO DEL PULSADOR
   PC13 usa pull-up:
   PRESIONADO = RESET
   LIBERADO = SET

   Antirrebote: 30 ms
   Pulsacion larga: 1500 ms
   ========================================================= */

void Button_Process(uint32_t currentMillis)
{
    /* Limpiar la bandera de interrupcion */
    if (g_extiTriggered != 0U)
    {
        g_extiTriggered = 0U;
    }

    /* Leer el estado actual del pin */
    GPIO_PinState rawState =
        HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

    /*
       Si cambia el nivel del pin, reiniciar
       el tiempo de antirrebote.
    */
    if (rawState != candidateButtonState)
    {
        candidateButtonState = rawState;
        debounceStart = currentMillis;
    }

    /*
       Aceptar el cambio solo despues de 30 ms
       con el mismo nivel.
    */
    if ((candidateButtonState != stableButtonState) &&
        ((uint32_t)(currentMillis - debounceStart) >= 30U))
    {
        stableButtonState = candidateButtonState;

        /* El pulsador acaba de ser presionado */
        if (stableButtonState == GPIO_PIN_RESET)
        {
            btnState = BTN_PRESSED;
            btnPressTime = currentMillis;
            longPressReported = 0U;
        }
        else
        {
            /* El pulsador acaba de ser liberado */
            if ((btnState == BTN_PRESSED) &&
                (longPressReported == 0U))
            {
                uint32_t pressDuration =
                    currentMillis - btnPressTime;

                if (pressDuration < 1500U)
                {
                    buttonActionFlag = 1U;
                }
            }

            btnState = BTN_IDLE;
        }
    }

    /*
       Detectar la pulsacion larga una sola vez,
       sin esperar a que se suelte el pulsador.
    */
    if ((btnState == BTN_PRESSED) &&
        (longPressReported == 0U))
    {
        if ((uint32_t)(currentMillis - btnPressTime) >= 1500U)
        {
            buttonActionFlag = 2U;
            longPressReported = 1U;
        }
    }
}

/* =========================================================
   CONTROL DE ESTADOS Y RAMPA AUTOMATICA
   ========================================================= */

void App_ProcessState(uint32_t now)
{
    /* -----------------------------------------------------
       PULSACION CORTA: CAMBIAR DE MODO
       ----------------------------------------------------- */

    if (buttonActionFlag == 1U)
    {
        buttonActionFlag = 0U;

        switch (currentSystemState)
        {
            case MODE_MANUAL25:
                currentSystemState = MODE_MANUAL40;
                break;

            case MODE_MANUAL40:
                currentSystemState = MODE_MANUAL60;
                break;

            case MODE_MANUAL60:
                currentSystemState = MODE_MANUAL80;
                break;

            case MODE_MANUAL80:
                currentSystemState = MODE_AUTO;
                autoDuty = 10U;
                rampDirection = 1;
                lastRampTick = now;
                break;

            case MODE_AUTO:
                currentSystemState = MODE_MANUAL25;
                break;

            case MODE_PAUSE:
            default:
                break;
        }
    }

    /* -----------------------------------------------------
       PULSACION LARGA: PAUSA Y REANUDACION
       ----------------------------------------------------- */

    if (buttonActionFlag == 2U)
    {
        buttonActionFlag = 0U;

        if (currentSystemState != MODE_PAUSE)
        {
            stateBeforePause = currentSystemState;
            currentSystemState = MODE_PAUSE;
        }
        else
        {
            currentSystemState = stateBeforePause;

            /* Evitar avanzar la rampa durante la pausa */
            lastRampTick = now;
        }
    }

    /* -----------------------------------------------------
       RAMPA AUTOMATICA ENTRE 10 % Y 90 %
       Un incremento cada 20 ms
       ----------------------------------------------------- */

    if (currentSystemState == MODE_AUTO)
    {
        while ((uint32_t)(now - lastRampTick) >= 20U)
        {
            lastRampTick += 20U;

            if (rampDirection > 0)
            {
                if (autoDuty < 90U)
                {
                    autoDuty++;
                }

                if (autoDuty >= 90U)
                {
                    autoDuty = 90U;
                    rampDirection = -1;
                }
            }
            else
            {
                if (autoDuty > 10U)
                {
                    autoDuty--;
                }

                if (autoDuty <= 10U)
                {
                    autoDuty = 10U;
                    rampDirection = 1;
                }
            }
        }
    }

    /* -----------------------------------------------------
       APLICAR EL PWM SEGUN EL ESTADO
       ----------------------------------------------------- */

    switch (currentSystemState)
    {
        case MODE_MANUAL25:
            Set_PWM_Duty(TIM_CHANNEL_3, 25U);
            break;

        case MODE_MANUAL40:
            Set_PWM_Duty(TIM_CHANNEL_3, 40U);
            break;

        case MODE_MANUAL60:
            Set_PWM_Duty(TIM_CHANNEL_3, 60U);
            break;

        case MODE_MANUAL80:
            Set_PWM_Duty(TIM_CHANNEL_3, 80U);
            break;

        case MODE_AUTO:
            Set_PWM_Duty(TIM_CHANNEL_3, autoDuty);
            break;

        case MODE_PAUSE:
            Set_PWM_Duty(TIM_CHANNEL_3, 0U);
            break;

        default:
            Set_PWM_Duty(TIM_CHANNEL_3, 0U);
            break;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_TIM4_Init();
    MX_USART2_UART_Init();

    /*Prepara al stm32 para recibir un caracter por USART2*/

    if (HAL_UART_Receive_IT(&huart2, &rxByte, 1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN 2 */

    /* Iniciar PWM en TIM2, canal 3 */
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    /* Iniciar temporizador de 1 ms */
    if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
    {
        Error_Handler();
    }

    /* Estado inicial */
    currentSystemState = MODE_MANUAL25;
    stateBeforePause = MODE_MANUAL25;

    autoDuty = 10U;
    rampDirection = 1;
    lastRampTick = 0U;

    Set_PWM_Duty(TIM_CHANNEL_3, 25U);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    uint32_t lastTaskTime = g_msTicks;

    while (1)
    {
        uint32_t currentMillis = g_msTicks;

        /* Procesamiento principal cada 1 ms */
        if ((uint32_t)(currentMillis - lastTaskTime) >= 1U)
        {
            lastTaskTime = currentMillis;

            Button_Process(currentMillis);
            App_ProcessState(currentMillis);
        }

        /* Telemetria UART cada 1000 ms */
        if ((uint32_t)(currentMillis - lastTelemetryTick) >= 1000U)
        {
            lastTelemetryTick = currentMillis;

            char buffer[100];
            const char *stateName;

            switch (currentSystemState)
            {
                case MODE_MANUAL25:
                    stateName = "MANUAL25";
                    break;

                case MODE_MANUAL40:
                    stateName = "MANUAL40";
                    break;

                case MODE_MANUAL60:
                    stateName = "MANUAL60";
                    break;

                case MODE_MANUAL80:
                    stateName = "MANUAL80";
                    break;

                case MODE_AUTO:
                    stateName = "AUTO";
                    break;

                case MODE_PAUSE:
                    stateName = "PAUSA";
                    break;

                default:
                    stateName = "DESCONOCIDO";
                    break;
            }

            int length = sprintf(
                buffer,
                "Estado: %s | autoDuty: %u%%\r\n",
                stateName,
                (unsigned int)autoDuty
            );

            if (length > 0 && length < (int)sizeof(buffer))
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)buffer,
                    (uint16_t)strlen(buffer),
                    10
                );
            }
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_3) != HAL_OK)
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
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 99U;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999U;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if (HAL_TIM_ConfigClockSource(
            &htim2,
            &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim2,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0U;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim2,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim2,
            &sConfigOC,
            TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim2,
            &sConfigOC,
            TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 99U;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 999U;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if (HAL_TIM_ConfigClockSource(
            &htim4,
            &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim4,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200U;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* LED integrado LD2 */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_EXTI13_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(GPIO_EXTI13_GPIO_Port, &GPIO_InitStruct);

    /* Configurar LED integrado */
    GPIO_InitStruct.Pin = LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

    /* Interrupcion EXTI para PC13 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* El usuario puede agregar aqui el reporte del error */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
