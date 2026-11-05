# Збірка та коротка перевірка

У звичайному терміналі спочатку перейти до кореня саме цього проєкту командою `cd`. Далі всі шляхи відносні; jump-аліаси не потрібні.

```sh
make build
make test
make coverage
```

`make build` створює disposable `media/demo-sd.img`, якщо його немає, та ARMартефакти в `firmware/build/`. Існуючий образ зберігається. У VS Code відкрити цейкорінь, F1 -> Wokwi: Start Simulator. Не активувати trial; використати наявну ліцензію.

Для збору UART потрібен pyserial в обраному Pythonсередовищі. Після налаштування залежності виконувати зкореня:

```sh
python3 tools/monitor.py --seconds 8 --command selftest --output docs/evidence/demo-uart.raw
python3 tools/run_lab2_checks.py --output docs/evidence/demo-integration
```

Команди bench: `selftest`, `status`/`read`, `tare`, `anomaly`, `fault`, `recover`, `counts`, `sdprobe`, `sdread`. `anomaly` звертається до 0x69одинраз; `fault` залишає неправильну адресу до `recover`. `sdprobe` змінює лише сектор 4096 власного disposableобразу. `hxoff/hxon` - додатковий діагностичний powerdownексперимент; recovery моделі в ньому не підтверджено. `portion/accept/reject/export` належать майбутньому продукту ЛР 3 і тут повертають ERR.

Для аналізу власного захвату:

```sh
python3 tools/analyze_capture.py docs/captures/lab2-final-sensor-checks.vcd --output docs/evidence/demo-signals.json
python3 tools/analyze_sd_capture.py docs/captures/lab2-sd-final.vcd --output docs/evidence/demo-sd-signals.json
```

Не видавати hostтести за перевірку плати, а короткий benchзапуск - за S1/S2/S3. СценаріївідеотаопераційнийстанзберігаютьсявлокальномуGUIDE, позаGit.
