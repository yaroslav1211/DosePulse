# DosePulse

Водопʼянов Ярослав Сергійович, ІО-32. Гравіметричний облік порцій: тара, ціль, маса та стабільність вагової платформи. Nucleo-C031C6; Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

## Перевірений обсяг

- `lab1`: PRD, компоненти, відтворювана Docker-збірка, справжні UART/LED і VCD. [Звіт](docs/report.md), PDF у `docs/reports/`.
- `lab2`: одночасний HX711, MPU6050, ADC; фільтри, self-test, помилки й відновлення I2C; фактичні SD block write/readback; host-тести, покриття, сигнали та аналіз. [Звіт](docs/lab2-report.md), [записи](docs/captures/analysis.md), PDF у `docs/reports/`.
- ЛР3–4: [вимоги](docs/PRD.md), [план експериментів](docs/tasks-and-experiments.md) та [bring-up сховища](docs/storage-bringup.md). Автомат порцій, журнал із CRC, RTOS і годинний S2 ще не реалізовані.

Результати отримано в симуляторі; фізичне живлення, точність ваги й апаратний I2C1 не перевірені. C031 I2C моделі має обмеження: стандартний bench використовує GPIO-адаптер на PB8/PB9. Додатковий дослід `hxoff/hxon` не відновив READY; це збережений негативний результат. Відео студентського захисту та pull request залишаються окремими кроками здавання.

## Відтворення

Відкрити термінал у корені цього репозиторію та виконати:

```sh
make build
make test
make coverage
```

ARM-файли: `firmware/build/firmware.elf`, `.hex`, `.bin`. `make build` створює власний disposable-образ `media/demo-sd.img`, якщо він відсутній. Відкрити цей корінь у VS Code, F1 → Wokwi: Start Simulator. Зберегти наявну ліцензію. Для UART потрібен Python із pyserial; [команди](docs/command-guide.md) виконуються з цього самого кореня.

[Pinout](docs/pinout.md), [джерела компонентів](docs/hardware-components.md), [архітектура](docs/architecture.md), [тести](docs/testing.md), [походження заготовки](docs/starter-template.md), [версії залежностей](docs/vendor-versions.json).

Фінальні докази: `docs/evidence/lab2/integration-delivery.*`, `sd-probe.*`, `delivery-signal-analysis.json`, `sd-signal-analysis.json`, `coverage/metrics.json`, `firmware-clean-sha256.txt`. Невдалі й попередні запуски збережено з окремими назвами; вони не є підтвердженням успішної фінальної перевірки.
