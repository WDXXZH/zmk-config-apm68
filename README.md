# APM68 ZMK 配置草案

这是从上传的 QMK 配置转换出的 ZMK 草案，包含矩阵、默认键位、物理布局、EC11 编码器和 68 颗 WS2812 underglow。

## 当前结论

这不是一个已经确认能直接刷入 APM32F103C8 的固件仓库。ZMK 基于 Zephyr，主线 ZMK 支持的键盘/控制器列表没有把 APM32F103C8 作为现成目标；本仓库暂时用 `bluepill_f103c8` 作为近似构建目标。如果 GitHub Actions 报 `board not found` 或 USB/时钟/WS2812 相关错误，需要先补 APM32F103C8 或 STM32F103C8 的 Zephyr/ZMK 板级支持。

## 文件说明

- `build.yaml`：GitHub Actions 的构建矩阵，目前暂用 `bluepill_f103c8 + apm68`。
- `config/apm68.keymap`：从 QMK `keymap.c` 转换的两层键位。
- `config/apm68.conf`：启用 RGB underglow、WS2812、EC11 编码器。
- `config/boards/shields/apm68/apm68.overlay`：矩阵引脚、编码器、WS2812 和 matrix transform。
- `config/boards/shields/apm68/apm68-layouts.dtsi`：给 keymap-editor / ZMK physical layout 使用的物理键位。
- `config/boards/shields/apm68/apm68.zmk.yml`：硬件元数据。

## 上传到 GitHub

1. 新建一个 GitHub 仓库，例如 `zmk-config-apm68`。
2. 把本目录全部提交并推送。
3. 打开 Actions 看构建结果。
4. 打开 https://nickcoutsos.github.io/keymap-editor/ ，授权 GitHub，并选择这个仓库编辑 `config/apm68.keymap`。

## 需要实机验证的点

- APM32F103C8 是否能完全按 STM32F103C8 的 Zephyr 板级配置运行。
- USB HID 枚举、时钟树、DFU/烧录方式是否与 QMK 当前固件一致。
- B12/B13 编码器方向是否需要交换。
- B14 上的 WS2812 时序在 Zephyr GPIO bitbang 下是否稳定；如不稳定，需要改成 PWM/SPI 驱动方案。
- `config/apm68.keymap` 当前按 QMK `keymap.c` 保留 69 个逻辑位置，其中 RGB LED 数量是 68。
