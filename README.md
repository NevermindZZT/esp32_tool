# ESP32 Tool

ESP32 开发调试工具，基于 ESP32-S3 芯片，手表形态，配备 ST7789V 显示屏。

## 技术栈

- **MCU:** ESP32-S3, Xtensa LX7 双核 @240MHz
- **框架:** ESP-IDF v6.0 (已从 v5.5 迁移)
- **显示:** ST7789V, 240×280, SPI
- **触摸:** CST816T 电容触摸
- **图形:** LVGL v9.5.0
- **内存:** 16MB Flash + Octal PSRAM (80MHz)
- **音频:** I2S MAX98357A 功放
- **USB:** TinyUSB (CDC + MSC)

## 拉取

```sh
git clone https://github.com/NevermindZZT/esp32_tool
git submodule update --init --recursive
git submodule update --recursive --remote
```

## 构建

```sh
# 使用 ESP-IDF v6.0 环境
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 从 ESP-IDF v5.5 迁移到 v6.0

项目已从 ESP-IDF v5.5 迁移到 v6.0，涉及以下变更：

### 已处理的迁移

1. **`main/idf_component.yml`**: IDF 版本约束改为 `>=6.0.0`
2. **`components/led/CMakeLists.txt`**: 修复了 CMakeLists 文件（原为 `.bak`）
3. **`sdkconfig`**: 添加 `CONFIG_I2C_SUPPRESS_DEPRECATE_WARN=y` 抑制 I2C 遗留驱动的 EOL 警告

### 在菜单配置中需确认的项

运行 `idf.py menuconfig` 后确认以下配置：

| 配置项 | 建议 |
|--------|------|
| `I2C Suppress Legacy Deprecated Warning` | 启用 (或迁移到新 I2C 驱动) |
| `Compiler option > Disable default errors` | 启用 (因 6.0 将警告视为错误) |
| `VFS Support TERMIOS` | 启用 (`usb_device.c` 使用 termios) |
| `Log version` | 可选迁移到 v2 API |

### 已知兼容性说明

- **I2C 驱动**: 项目使用遗留 I2C 驱动 (`driver/i2c.h`)，该驱动在 v6.0 标记为 EOL，将在 v7.0 移除。当前通过编译选项抑制警告。涉及的组件：`protocol/serial_debug_i2c.c`、`multimeter/ina226.c`
- **ADC**: 项目已使用新 ADC 驱动 API (`adc_oneshot_*`)，无需迁移
- **VFS**: `usb_device.c` 中的 `esp_vfs_dev_cdcacm_register()` 和 `esp_vfs_open()` 仅在 `CONFIG_ESP_CONSOLE_USB_CDC` 启用时编译，当前未启用
- **子模块**: 所有子模块（lvgl、letter-shell、rtam、cpost、lvgl_esp32_drivers）均使用指向兼容版本的固定提交
