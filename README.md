# DosePulse

Водопʼянов Ярослав Сергійович, ІО-32. Гравіметричний облік порцій: тара, ціль, маса та стабільність вагової платформи. Nucleo-C031C6; Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

## ЛР1
`make build` створює `firmware/build/firmware.elf`, `.hex`, `.bin`. Дві чисті Docker-збірки мають однакові SHA256: `docs/evidence/lab1/hashes-a.txt` і `hashes-b.txt`. `make flash` показує спосіб запуску через Wokwi VS Code; F1 -> Wokwi: Start Simulator. UART115200 і LEDPA5 перевірено окремим запуском, збережено `docs/evidence/lab1/uart.raw` та `docs/captures/lab1-uart-led.vcd`. Для автоматичного збору: Python3+pyserial, `python3 tools/monitor.py --seconds 8 --output docs/evidence/demo-uart.raw`.

[PRD](docs/PRD.md), [компоненти](docs/hardware-components.md), [походження заготовки](docs/starter-template.md), [план експериментів ЛР3-4](docs/tasks-and-experiments.md). Драйвери належать ЛР2; повний автомат порцій, постійний журнал та RTOS тут не реалізовані. Фізичного обладнання не перевірено.
