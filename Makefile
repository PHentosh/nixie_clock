BUILD_DIR           ?= build
BUILD_TYPE          ?= Debug
CMAKE_GENERATOR     ?= Ninja
SDK                 ?= esp-idf

.PHONY: all generate app clean clean-all

all: app

generate:
ifeq ($(filter $(BUILD_TYPE),Debug Release), )
	$(error Unknown BUILD_TYPE: "$(t_b)$(BUILD_TYPE)$(t_n)". Valid types: "Debug", "Release")
else
	$(info BUILD_TYPE is $(t_b)$(BUILD_TYPE)$(t_n))
endif

ifeq ($(SDK), esp-idf)
	$(info SDK is $(t_b)ESP-IDF$(t_n))
	$(if $(IDF_PATH),,$(error Please export ESP-IDF framework before run make for this SDK))
	$(eval CMAKE_TOOLCHAIN_FILE = $(IDF_PATH)/tools/cmake/toolchain-esp32.cmake)
	$(eval CHIP ?= esp32)
else
	$(error Unknown SDK: "$(SDK)"! Please pass $(t_b)SDK$(t_n) variable)
endif

	@cmake . -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) -D CMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) \
		-D CMAKE_BUILD_TYPE=$(BUILD_TYPE)

app:
	@cmake --build $(BUILD_DIR)

flash:
	@cmake --build $(BUILD_DIR) -- flash

monitor:
	@cmake --build $(BUILD_DIR) -- monitor

clean:
	@cmake --build $(BUILD_DIR) -- clean

clean-all:
	@echo "  RM $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)
