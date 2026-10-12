# Documentación de hardware y periféricos

## 1. Microcontrolador

- Placa: NUCLEO-F411RE.
- Microcontrolador: STM32F411RETx.
- Entorno de desarrollo: STM32CubeIDE.
- Lenguaje: C.
- Biblioteca de control: STM32 HAL.

## 2. GPIO

### PC13 — Botón físico

- Función: entrada digital con interrupción externa.
- Configuración visible en CubeMX: GPIO_EXTI13.
- Uso: detectar las pulsaciones del botón físico.
- Lógica del programa: pulsación activa en nivel bajo, con antirrebote y detección de pulsación larga.

### Otros pines

La captura de CubeMX muestra las siguientes asignaciones:

| Pin | Función indicada en CubeMX |
|---|---|
| PA0 | TIM2_CH1 |
| PA1 | TIM2_CH2 |
| PA2 | USART_TX |
| PA3 | USART_RX |
| PA5 | GPIO_Output |
| PH0 | RCC_OSC_IN |
| PH1 | RCC_OSC_OUT |

Estas asignaciones deben contrastarse con la configuración completa y el cableado real antes de atribuirles una función física definitiva.

## 3. Control PWM

- Temporizador: TIM2.
- Canal utilizado por la rutina de inicio del programa: canal 3.
- Función: regular el brillo mediante la variación del ciclo de trabajo de la señal PWM.
- Función HAL de inicio: HAL_TIM_PWM_Start.
- Función de ajuste: __HAL_TIM_SET_COMPARE.

El ciclo de trabajo se modifica mediante el valor de comparación del temporizador. Un ciclo de trabajo mayor normalmente aumenta el brillo del LED, siempre que el circuito de salida tenga la polaridad esperada.

## 4. Temporizador TIM4

TIM4 genera interrupciones periódicas utilizadas por el programa para mantener una referencia de tiempo en milisegundos.

La variable global g_msTicks se incrementa en la rutina de interrupción del temporizador. Este contador se utiliza para temporizar tareas como el antirrebote del botón y la transmisión periódica de información.

El período exacto debe verificarse en la configuración de TIM4 y en los parámetros del reloj.
Prescaler: 99



Counter Period (ARR): 999



Counter Mode: Up



Reloj del temporizador: 100 MHz, según la configuración que revisamos



Período calculado: 1 ms



Interrupción global de TIM4: habilitada
## 5. Comunicación UART

- Periférico: USART2.
- Velocidad: 115200 baudios.
- Formato: 8 bits de datos, sin paridad y 1 bit de parada.
- Recepción: mediante interrupción de recepción de un byte.
- Transmisión: envío de información de estado y telemetría.

### Comandos

| Carácter recibido | Acción |
|---|---|
| 1 | Iluminación al 25 % |
| 2 | Iluminación al 40 % |
| 3 | Iluminación al 60 % |
| 4 | Iluminación al 80 % |
| A o a | Modo automático |
| P o p | Pausar o reanudar |

## 6. Temporización y control del botón

El programa utiliza una referencia temporal en milisegundos para implementar un antirrebote de aproximadamente 30 ms. Las pulsaciones cortas cambian el modo de funcionamiento y las pulsaciones largas permiten pausar o reanudar el sistema.

## 7. Verificación pendiente

Antes de considerar esta documentación definitiva, se deben comprobar:

- El pin físico de salida de TIM2_CH3.
- El período y el prescaler configurados en TIM2 y TIM4.
- Los parámetros del reloj del microcontrolador.
- La correspondencia entre los pines UART y el cableado utilizado.
- El comportamiento real de la salida PWM y del botón.

Las evidencias de estas verificaciones se guardan en la carpeta evidencias del repositorio.