IMAGE ?= stm32c0-wokwi-build:local

.PHONY: image build clean

image:
	docker build -t $(IMAGE) docker

build: image
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
