# Controlador de Iluminación con STM32F411RE

## 1. Descripción del proyecto

Este proyecto implementa un sistema de control de iluminación mediante un microcontrolador STM32F411RE. Permite seleccionar diferentes niveles de brillo, activar un modo automático y controlar el sistema mediante un botón físico y una comunicación serial UART.

El programa está desarrollado en lenguaje C utilizando STM32CubeIDE y las bibliotecas HAL de STMicroelectronics.

## 2. Objetivos

- Controlar el brillo de una salida mediante PWM.
- Seleccionar niveles de iluminación mediante un botón físico.
- Incorporar un modo automático de funcionamiento.
- Permitir el control mediante comandos enviados por UART.
- Transmitir información del estado del sistema por comunicación serial.

## 3. Hardware y herramientas

- Placa de desarrollo STM32 Nucleo-F411RE.
- Microcontrolador STM32F411RE.
- LED externo y su resistencia limitadora, según el montaje utilizado.
- Botón físico conectado a una entrada GPIO.
- Computadora con STM32CubeIDE.
- Terminal serial, por ejemplo PuTTY.
- Cable USB para programar y depurar la placa.

## 4. Funcionamiento

El sistema dispone de cuatro niveles de iluminación manual: 25 %, 40 %, 60 % y 80 %. También cuenta con un modo automático y una función de pausa.

El botón físico permite cambiar entre los modos mediante pulsaciones cortas y pausar o reanudar el funcionamiento mediante una pulsación larga.

La comunicación UART permite seleccionar los modos de funcionamiento y recibir información periódica del estado del sistema.

## 5. Comandos seriales

| Comando | Función |
|---|---|
| 1 | Seleccionar iluminación al 25 % |
| 2 | Seleccionar iluminación al 40 % |
| 3 | Seleccionar iluminación al 60 % |
| 4 | Seleccionar iluminación al 80 % |
| A o a | Activar el modo automático |
| P o p | Pausar o reanudar el sistema |

Configuración serial: 115200 baudios, 8 bits de datos, sin paridad y 1 bit de parada (115200, 8-N-1).

## 6. Periféricos del microcontrolador

- GPIO: lectura del botón físico.
- TIM2, canal 3: generación de la señal PWM.
- TIM4: interrupción periódica para la referencia de tiempo del programa.
- USART2: recepción de comandos y transmisión de telemetría.

Los pines físicos, las funciones alternativas y los parámetros completos de los periféricos se documentan en `documentacion/configuracion_hardware.md`, tomando como referencia la configuración real del proyecto.

## 7. Estructura del repositorio

- `Core/`: código principal y configuración del proyecto.
- `Drivers/`: bibliotecas y controladores utilizados.
- `ControladorIluminacionconconsolaserial.ioc`: configuración de STM32CubeMX.
- `documentacion/`: documentación técnica.
- `evidencias/`: capturas y pruebas del funcionamiento.

## 8. Programación y ejecución

1. Abrir el proyecto en STM32CubeIDE.
2. Verificar la configuración del microcontrolador en el archivo `.ioc`.
3. Compilar el proyecto.
4. Conectar la placa y programar el microcontrolador.
5. Abrir una terminal serial a 115200 baudios para enviar comandos y observar la telemetría.
6. Comprobar el funcionamiento del botón y de los niveles de iluminación.

## 9. Evidencias

Las capturas de configuración, compilación y funcionamiento se encuentran en la carpeta `evidencias/`.

## 10. Autoría

Proyecto académico de microcontroladores. El código fuente, la configuración y las evidencias corresponden al desarrollo del proyecto.