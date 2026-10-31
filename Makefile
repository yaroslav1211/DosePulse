IMAGE ?= stm32c0-wokwi-build:local

.PHONY: image build clean

image:
	docker build -t $(IMAGE) docker

build: image demo-media
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

.PHONY: flash monitor
flash:
	@echo "VS Code: Wokwi: Start Simulator (uses wokwi.toml ELF/HEX)"

monitor:
	python3 tools/monitor.py

.PHONY: demo-media test
HOST_CC ?= cc
demo-media:
	python3 tools/init_demo_media.py

test:
	python3 tools/check_dependencies.py
	python3 tools/check_diagram.py
	mkdir -p build-host
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Ifirmware/Devices -Ifirmware/App tests/test_sensors.c firmware/Devices/dp_sensors.c firmware/App/dp_quality.c -o build-host/test-sensors
	./build-host/test-sensors
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Ifirmware/Devices tests/test_sd.c firmware/Devices/dp_sd.c -o build-host/test-sd
	./build-host/test-sd

.PHONY: coverage
coverage:
	python3 tools/coverage.py
