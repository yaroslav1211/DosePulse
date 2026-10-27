# DosePulse: план компонентів

- HX711 / GPIO DT-SCK: маса
- MPU6050 / I²C: рух вагової платформи
- потенціометр / ADC: цільова маса порції

Наведено заплановані входи; вони ще не додані до `diagram.json`, `.ioc` або прошивки. Початкова плата — STM32C031C6, HAL STM32C0. Базова схема має UART на PA2/PA3; остаточний pinout GPIO/I²C/ADC треба перевірити перед змінами.

Для кожного компонента в ЛР1 потрібно знайти офіційний datasheet / register map і додати конкретні сторінки: живлення, рівні сигналів, протокол, адреса/ідентифікація, перетворення одиниць, частота та час вимірювання, помилки. Наявність моделі Wokwi не замінює datasheet. GPIO-рівні зовнішніх 5-V моделей не є доказом електричної сумісності фізичного стенда.

Переглянуті джерела моделювання:

- [Доступні компоненти Wokwi](https://docs.wokwi.com/getting-started/supported-hardware)
- [Nucleo-C031C6](https://docs.wokwi.com/parts/board-st-nucleo-c031c6)
- [BMP180](https://docs.wokwi.com/parts/board-bmp180), [MPU6050](https://docs.wokwi.com/parts/wokwi-mpu6050)
- [HX711](https://docs.wokwi.com/parts/wokwi-hx711), [HC-SR04](https://docs.wokwi.com/parts/wokwi-hc-sr04)
- [DHT22](https://docs.wokwi.com/parts/wokwi-dht22), [NTC](https://docs.wokwi.com/parts/wokwi-ntc-temperature-sensor)
- [DS18B20](https://docs.wokwi.com/parts/wokwi-ds18b20)

## Межі

Рух і маса в Wokwi незалежні; автоматичне дозування приводом не обіцяється. Калібрування маси й допустима нестабільність потребують вимірювань.

Порівняти альтернативи до вибору pinout: цифровий та аналоговий вимір, окремий контакт замість непрямої ознаки, UART-генератор замість фізичного входу. У PRD вказати роль кожного реально обраного компонента, а не додавати пристрій лише заради кількості.
