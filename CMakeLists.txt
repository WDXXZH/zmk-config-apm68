# Copyright (c) 2024 APM68 Keyboard
# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_LED_STRIP_WS2812_STM32
  drivers/led_strip/ws2812_stm32.c
)
