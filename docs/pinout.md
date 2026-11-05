# DosePulse: підключення

Плата Nucleo-C031C6, STM32C031C6 LQFP48. Джерела: [DS13867](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), табл.12,13,15; [UM2953](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf), Fig.7 та табл.11, с.19-20; [RM0490](https://www.st.com/resource/en/reference_manual/rm0490-stm32c0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), RCC/ADC/I2C/USART/TIM.

| MCU | Режим / AF | Роз'єм | Пристрій / рівень / підтягування | Чому цей пін |
|---|---|---|---|---|
| PA0 | analog ADC1_IN0 | CN8-1 A0 | target SIG 0..3.3V; no pull | Доступний аналоговий вхід, окремо від UART. |
| PA1 | GPIO input | CN8-2 A1 | HX711 DT; MCU pull-up3.3V | Готовність/дані без AF; вільний вивід. |
| PA8 | GPIO output | CN10 Fig.7 | HX711 SCK 3.3V push-pull; initial0 | Не займає майбутню SPIшину. |
| PB8 | HAL GPIO open-drain; доступна AF6 I2C1_SCL | CN5-10 D15 | IMU SCL; 4.7k до3.3V на стенді | Роз'єм I2C; AF підтверджено C031. |
| PB9 | HAL GPIO open-drain; доступна AF6 I2C1_SDA | CN5-9 D14 | IMU SDA; 4.7k до3.3V | Парний сигнал I2C, адреса0x68 при AD0=GND. |
| PA2/PA3 | USART2 AF1 TX/RX | morpho, Fig.7; не D1/D0 | serial monitor 1152008N1 | Збережено VCP заготовки; D1/D0 плати - PB6/PB7 USART1. |
| PA5 | GPIO output | CN5-6 D13 | вбудований LED | SPI SCK сюди не підключено. |
| PB0 | GPIO output | CN5-3 D10 | ADC marker / logic D4 | Вільний цифровий вихід; не SD CS. |
| PB3 | GPIO output, SPI mode0 bit-bang | CN9-4 D3 | SD SCK,3.3V | AF0 SPI1 доступна для майбутнього HALпереносу без зміни проводів. |
| PB4 | GPIO input | CN9-6 D5 | SD DO/MISO, MCU pull-up3.3V | Вільний MISO, не PA6. |
| PB5 | GPIO output | CN9-7 D6 | SD DI/MOSI,3.3V | Парний MOSI до PB3/PB4. |
| PA4 | GPIO output initial1 | CN8-3 A2 | SD CS active-low | Незалежний CS, не конфліктує з ADC0. |

З'єднати всі GND. IMU і потенціометр живити 3.3V. Wokwi HX711 VCC5V відповідно до моделі; у фізичному стенді DVDD має бути 3.3V, див.hardware-components.md. microSD -3.3V, окреме джерело з розрахунковим запасом>=200mA; ідеальне живлення Wokwi не є виміряним бюджетом. Підтягування I2C враховують наявні резистори модуля: дві пари 4.7k паралельно дають 2.35k, тому не додавати без перевірки. Для фізичної шини перевірити час наростання осцилографом і фактичну ємність.

HSI48MHz -> AHB48MHz -> APB48MHz. UART BRR≈48MHz/115200=416.67,16x oversampling. I2C TIMINGR0x20303E5D: PRESC2, tPRESC62.5ns, SCLL93, SCLH62: ідеальна сума 9.8125us, плюс апаратні затримки/фільтр; очікується близько 100kHz, порівняти з VCD. ADC PCLK/4=12MHz; sampling79.5+12.5cycles≈7.667us, службовий PB0 охоплює також HALнакладні витрати. TIM3 PSC47 ->1MHz, ARR65535; використовується тільки коротке usочікування. SPI - власний GPIO mode0, дві паузи 2us на біт плюс HAL; початкова частота<=250kHz, фактичну взяти з VCD. Для готового SDносія немає WREN: використано команди SD, а не SPI NORflash.

## Відхилені розкладки та конфлікти
1. SPI1 PA5/PA6/PA7 відхилено: PA5 уже LED, тому перенесення на PB3/4/5 зберігає індикацію.
2. HX711 на PA2/PA3 відхилено через USART2; альтернативний ADC PA4 відхилено через SD CS. PA0 відокремлює ціль від SD.
SWD PA13/14, PC13кнопка та кварцові PF0/1, PC14/15 не використовуються сенсорами. DMAпередач немає, конфліктів каналів немає. TIM3 має одну роль. Фізичної камери немає у форміB; змін MCU для неї не зроблено.

## Аналізатор
D0=PB8SCL, D1=PB9SDA, D2=PA8HX_SCK, D3=PA1HX_DT, D4=PB0ADCmarker, D5=PA2UART_TX, D6=PB3SD_SCK, D7=PA4SD_CS. Для повного SDcapture тимчасово D0=PB5MOSI, D1=PB4MISO; зберегти окремий diagram/input і VCD. VCD - подієвий, шкала 1ns; sigrok downsample500 дає 2MHz, щонайменше 8samples/bit при 250kHz. Тригер - запуск симуляції, зупинка після потрібних команд. Змінювати файл latest замість збережених вимірів.

У фінальному Wokwi bench PB8/PB9 керуються HAL GPIO open-drain, а не апаратним I2C. Початковий HALI2C зависав післяACK (ISR0x8043, HAL_ERROR_TIMEOUT), як у wokwi-features#744. VCDневдалихспробзбережено. Частота задається 5usпаузами плюсGPIOнакладнівитрати; початковийрозрахунокTIMINGRописуєвідхиленуHWспробу. СлабкіMCUpull-upвключеноразоміз 4.7k. РейкиWokwiмаютьточніімена 3V3.1і 5V.1;іменабезсуфіксаневалідні.
