# APM68 ZMK 配置

APM68 键盘(APM32F103C8,兼容 STM32F103C8)的 ZMK 固件配置。
通过 GitHub Actions 自动构建固件,可通过 [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) 在线编辑键位。

## 结构说明

```
.
├── .github/workflows/build.yml       # GitHub Actions 构建工作流
├── build.yaml                        # 构建矩阵(board + shield)
├── config/
│   ├── west.yml                      # ZMK west 清单(拉取 ZMK 源码)
│   ├── apm68.keymap                  # 键位(两层: Base / Fn)
│   └── apm68.conf                    # 配置(RGB 禁用 + EC11 编码器)
├── boards/others/apm32_f103c8/       # 自定义板定义(基于 STM32F103)
└── boards/shields/apm68/             # 键盘 shield 定义
    ├── apm68.overlay                 # 矩阵引脚、编码器、matrix transform
    ├── apm68-layouts.dtsi            # 物理布局(供 Keymap Editor / Studio 使用)
    ├── apm68.zmk.yml                 # 硬件元数据
    ├── Kconfig.shield
    └── Kconfig.defconfig
```

## 工作原理

- `config/west.yml` 声明从 `zmkfirmware/zmk` 拉取 ZMK 源码。
- `zephyr/module.yml` 把本仓库声明为 ZMK 模块,`boards/` 作为板/盾根目录。
- GitHub Actions 使用 ZMK 官方的 `build-user-config.yml` 构建 `apm32_f103c8 + apm68`。
- 构建产物为 `.bin` 文件,可在 Actions 页面下载。

## 使用 Keymap Editor

1. 打开 [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)。
2. 授权 GitHub,选择 `WDXXZH/zmk-config-apm68` 仓库。
3. 编辑 `config/apm68.keymap` 中的键位。
4. 保存后提交,会自动触发 GitHub Actions 重新构建。

## 固件烧录

- 构建产物: Actions → 最新一次构建 → Artifacts → 下载 `firmware`。
- 烧录方式: 使用 ST-Link(OpenOCD)或 DFU 模式写入 APM32F103C8。
  - OpenOCD: `openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program apm68-apm32_f103c8-zmk.bin 0x08000000 verify reset exit"`
  - 或使用 QMK 相同的烧录方式(SWD / 串口 ISP)。

## 当前状态

- ✅ GitHub Actions 构建通过(apm32_f103c8 + apm68)。
- ✅ 键位与 QMK `keymap.c` 一致(69 键,2 层)。
- ✅ EC11 编码器绑定(音量控制)。
- ⚠️ RGB underglow 暂禁用:WS2812 数据脚 B14 是 SPI2 MISO,Zephyr 的 WS2812 GPIO 驱动仅支持 Nordic 系列,SPI 模式需要改硬件接线。后续可编写 STM32 自定义 led_strip 驱动启用。
- ⚠️ 实机验证项:USB 枚举、时钟树、编码器方向、DFU 烧录兼容性。
