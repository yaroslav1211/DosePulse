# Компоненти DosePulse

| Пристрій | Параметри | Джерело виробника |
|---|---|---|
| HX711 | GPIO DOUT ready-low; 24 MSB-first біти + 25-й SCK для A/128. ID немає. 10 SPS, settling400ms; SCK high0.2..50us, low>=0.2us; high>60us power-down. | [AVIA, копія SparkFun](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf), с.2 pinout, с.3 характеристики, с.4 живлення, с.5-6 протокол. |
| MPU6050 | I2C0x68 AD0=GND; WHO_AM_I0x75=0x68; 0x6b=0,0x1a=3,0x1b/1c=0,0x19=19. Burst14 від0x3b.16384LSB/g,131LSB/(deg/s). | [TDK register map rev4.2](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf), с.11,13-15,29-31,40,45; [spec rev3.4](https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6000-datasheet1.pdf), с.12-14,21-23,33-36. |
| Потенціометр | 10kOhm, кінці3.3V/GND, повзунок PA0 ADC1_IN0. 0..4095; mV=raw*3300/4095;10Hz. Фізичний аналог Bourns3386. | [Bourns3386](https://www.bourns.com/docs/Product-Datasheets/3386.pdf), с.1 опір/0.5W/допуски, с.2 схема повзунка. |

МК: [DS13867rev4](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), табл.12/13/15,с.30-36: LQFP48 та AF. Плата: [UM2953rev2](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf),с.12-16 живлення/VCP/LED; Fig.7 і табл.11,с.19-20 роз'єми. Периферія: [RM0490](https://www.st.com/resource/en/reference_manual/rm0490-stm32c0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), RCC/GPIO/ADC/I2C/USART/SPI. Приклади F4 не встановлюють pinout C031.

## Живлення
HX711 IC2.6..5.5V: analog1.4mA+digital0.1mA typ. DVDD фізичного модуля має відповідати MCU3.3V. Wokwi HX711 живиться5V за документацією моделі; реальний5-V модуль потребує DVDD3.3V або узгодження рівнів. MPU6050 VDD2.375..3.46V,3.9mA typ accel+gyro, pull-up до3.3V. Потенціометр10k:0.33mA,1.089mW. Міст1k потребував би ще3.3mA. Типова сума сенсорів близько9.1mA; резерв20mA без MCU/LED/карти. Пікові струми й запас регулятора потребують фізичного вимірювання. Майбутня microSD: окремий3.3-V регулятор із запасом>=200mA, спільна земля й локальні конденсатори; уточнити за реальною картою.

## Модель
[Wokwi HX711](https://docs.wokwi.com/parts/wokwi-hx711):5kg=2100counts;1count=2.38095g; gain/channelB не моделюються. Це калібровка моделі. Фізична вага потребує двоточкової калібровки. Маса та рух незалежні. Готові Arduino драйвери не використовуються. Альтернативи сенсорів наведено в PRD.

Пряме завантаження TDK у поточному середовищі дає403/HTML-перенаправлення; зміст і сторінки перевірено через доступне представлення документів. HTML не використовується як datasheet.
