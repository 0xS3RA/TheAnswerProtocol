BUILD_DIR ?= build
HOST ?= 127.0.0.1
PORT ?= 4242
WORLD ?= world.yaml
NAME ?=

.PHONY: all install build run-server run-client run-client-gui lint test clean

all: build

install:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build: install
	cmake --build $(BUILD_DIR) --parallel

run-server: build
	./$(BUILD_DIR)/bin/tap-server --host $(HOST) --port $(PORT) --world $(WORLD)

run-client: build
	./$(BUILD_DIR)/bin/tap-cli --host $(HOST) --port $(PORT) $(if $(NAME),--name $(NAME),)

run-client-gui:
	@echo "The GUI is intentionally not implemented in this server/CLI milestone." >&2
	@exit 1

lint: install
	cmake --build $(BUILD_DIR) --parallel
	@if command -v clang-format >/dev/null; then \
		clang-format --dry-run --Werror common/*.cpp common/*.hpp server/*.cpp server/*.hpp \
			cli/*.cpp tests/*.cpp; \
	else \
		echo "clang-format not found: strict -Wall -Wextra -Wpedantic -Werror build passed"; \
	fi

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)
