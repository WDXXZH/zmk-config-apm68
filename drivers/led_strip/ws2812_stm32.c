/*
 * Copyright (c) 2024 APM68 Keyboard
 *
 * SPDX-License-Identifier: MIT
 *
 * STM32F103 (APM32F103) WS2812 LED strip driver - GPIO bit-bang.
 *
 * Uses the Data Watchpoint and Trace (DWT) cycle counter for
 * nanosecond-accurate delays. Works on Cortex-M3 at 72MHz.
 */

#define DT_DRV_COMPAT worldsemi_ws2812_stm32

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/led/led.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_stm32);

/* Cortex-M cycle counter (DWT->CYCCNT) */
#define DWT_CTRL (*((volatile uint32_t *)0xE0001000))
#define DWT_CYCCNT (*((volatile uint32_t *)0xE0001004))
#define DEMCR (*((volatile uint32_t *)0xE000EDFC))
#define DEMCR_TRCENA (1 << 24)
#define DWT_CTRL_CYCCNTENA (1 << 0)

/* Core clock in Hz (72MHz for STM32F103 at full speed) */
#ifndef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 72000000
#endif

#define NS_TO_CYCLES(ns) (((uint64_t)(ns) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / 1000000000UL)

/* WS2812 timings (nanoseconds) */
#define T0H_NS 350
#define T1H_NS 700
#define T0L_NS 800
#define T1L_NS 600

struct ws2812_stm32_cfg {
    struct gpio_dt_spec gpio;
    uint8_t num_colors;
    const uint8_t *color_mapping;
    size_t length;
};

static void dwt_init(void)
{
    DEMCR |= DEMCR_TRCENA;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
    DWT_CYCCNT = 0;
}

static inline void delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT_CYCCNT;
    while ((DWT_CYCCNT - start) < cycles) {
    }
}
static int send_buf(const struct device *dev, uint8_t *buf, size_t len)
{
    const struct ws2812_stm32_cfg *cfg = dev->config;
    const uint32_t high_cycles = NS_TO_CYCLES(T1H_NS);
    const uint32_t low_cycles = NS_TO_CYCLES(T1L_NS);
    const uint32_t zero_high_cycles = NS_TO_CYCLES(T0H_NS);
    const uint32_t zero_low_cycles = NS_TO_CYCLES(T0L_NS);
    unsigned int key;

    key = irq_lock();

    for (size_t i = 0; i < len; i++) {
        uint32_t b = buf[i];
        for (int bit = 7; bit >= 0; bit--) {
            if (b & BIT(bit)) {
                /* 1 bit: T1H high, T1L low */
                gpio_pin_set_dt(&cfg->gpio, 1);
                delay_cycles(high_cycles);
                gpio_pin_set_dt(&cfg->gpio, 0);
                delay_cycles(low_cycles);
            } else {
                /* 0 bit: T0H high, T0L low */
                gpio_pin_set_dt(&cfg->gpio, 1);
                delay_cycles(zero_high_cycles);
                gpio_pin_set_dt(&cfg->gpio, 0);
                delay_cycles(zero_low_cycles);
            }
        }
    }

    irq_unlock(key);

    /* Reset delay to latch the strip */
    k_busy_wait(50);
    return 0;
}

static int ws2812_stm32_update_rgb(const struct device *dev,
                                   struct led_rgb *pixels,
                                   size_t num_pixels)
{
    const struct ws2812_stm32_cfg *cfg = dev->config;
    uint8_t *ptr = (uint8_t *)pixels;
    size_t i;

    /* Convert from RGB to on-wire format (GRB) */
    for (i = 0; i < num_pixels; i++) {
        uint8_t j;
        const struct led_rgb current_pixel = pixels[i];

        for (j = 0; j < cfg->num_colors; j++) {
            switch (cfg->color_mapping[j]) {
            case LED_COLOR_ID_WHITE:
                *ptr++ = 0;
                break;
            case LED_COLOR_ID_RED:
                *ptr++ = current_pixel.r;
                break;
            case LED_COLOR_ID_GREEN:
                *ptr++ = current_pixel.g;
                break;
            case LED_COLOR_ID_BLUE:
                *ptr++ = current_pixel.b;
                break;
            default:
                return -EINVAL;
            }
        }
    }

    return send_buf(dev, (uint8_t *)pixels, num_pixels * cfg->num_colors);
}

static size_t ws2812_stm32_length(const struct device *dev)
{
    const struct ws2812_stm32_cfg *cfg = dev->config;
    return cfg->length;
}

static DEVICE_API(led_strip, ws2812_stm32_api) = {
    .update_rgb = ws2812_stm32_update_rgb,
    .length = ws2812_stm32_length,
};

#define WS2812_COLOR_MAPPING(idx)                                    \
    static const uint8_t ws2812_stm32_##idx##_color_mapping[] =      \
        DT_INST_PROP(idx, color_mapping)

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#define WS2812_STM32_DEVICE(idx)                                     \
    WS2812_COLOR_MAPPING(idx);                                       \
                                                                     \
    static int ws2812_stm32_##idx##_init(const struct device *dev)   \
    {                                                                \
        const struct ws2812_stm32_cfg *cfg = dev->config;            \
                                                                     \
        if (!gpio_is_ready_dt(&cfg->gpio)) {                         \
            LOG_ERR("GPIO device not ready");                        \
            return -ENODEV;                                          \
        }                                                            \
                                                                     \
        dwt_init();                                                  \
        return gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT);       \
    }                                                                \
                                                                     \
    BUILD_ASSERT(WS2812_NUM_COLORS(idx) <= sizeof(struct led_rgb),   \
                 "Too many channels in color-mapping; "              \
                 "currently not supported");                         \
                                                                     \
    static const struct ws2812_stm32_cfg ws2812_stm32_##idx##_cfg = {\
        .gpio = GPIO_DT_SPEC_INST_GET(idx, gpios),                   \
        .num_colors = WS2812_NUM_COLORS(idx),                        \
        .color_mapping = ws2812_stm32_##idx##_color_mapping,         \
        .length = DT_INST_PROP(idx, chain_length),                   \
    };                                                               \
                                                                     \
    DEVICE_DT_INST_DEFINE(idx, ws2812_stm32_##idx##_init, NULL,      \
                          NULL, &ws2812_stm32_##idx##_cfg,           \
                          POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY, \
                          &ws2812_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_STM32_DEVICE)
