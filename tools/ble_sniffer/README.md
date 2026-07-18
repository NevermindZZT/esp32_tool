# BLE Sniffer — 使用文档

## 概述

ESP32-Tool 手表的 BLE 抓包器，通过 GAP 扫描捕获空中蓝牙广播包，支持：

- **实时 Wireshark 分析** — pcapng 格式通过 USB CDC 输出到 PC
- **btsnoop 文件录制** — 录制到 FAT 分区或 PC 端实时转换，用 Ellisys/Frontline 等分析
- **Shell 实时查看** — 命令行查看捕获的数据包摘要

## 使用方法

### 1. 启动抓包

手表上点击 **BLE Sniff** 应用 → 自动初始化 BT 和 GAP 扫描 → UI 显示抓包状态。

> 每次进入应用自动开始扫描，右滑返回自动停止并释放 BT 资源。

### 2. 通过 Wireshark 实时分析

#### 方法 A：管道桥接（推荐）

```bash
# 1. 找到 sniffer 的 COM 口
# 设备管理器 → 端口 → USB Serial Device (COMx)

# 2. 一键启动（编辑 tools/ble_sniffer/start_sniffer.cmd 里的 COM_PORT 后双击）
tools/ble_sniffer/start_sniffer.cmd

# 或手动执行：
python tools/ble_sniffer/esp_ble_sniffer_pipe.py COM4 | wireshark -k -i -
```

#### 方法 B：Wireshark extcap 插件

将 `tools/ble_sniffer/extcap/` 中的两个文件复制到 Wireshark 的 extcap 目录：

```bat
rem 以管理员身份运行
copy tools\ble_sniffer\extcap\esp_ble_sniffer_extcap.bat "%APPDATA%\Wireshark\extcap\"
copy tools\ble_sniffer\extcap\esp_ble_sniffer_extcap.py "%APPDATA%\Wireshark\extcap\"
```

或直接右键 `install_extcap.cmd` → **以管理员身份运行**。

重启 Wireshark 后捕获接口列表中出现 **"ESP32 BLE Sniffer"**。

### 3. btsnoop 文件录制

btsnoop 是行业标准的蓝牙抓包文件格式，可用 **Ellisys Bluetooth Analyzer**、**Frontline ComProbe** 等专业工具分析。

#### 方式 A：PC 端实时转换（推荐）

从 CDC 串口读取 pcapng 数据流，实时转换为 btsnoop 文件保存：

```bash
python tools/ble_sniffer/esp_ble_sniffer_to_btsnoop.py COM4 capture.btsnoop
```

程序运行中每 2 秒显示已转换的数据包数量，Ctrl+C 停止并保存文件。

#### 方式 B：手表端录制到 FAT 分区

在手表 Shell 中执行：

```
ble_sniff record                 # 开始录制到 /spiflash/capture.btsnoop
ble_sniff stoprec                # 停止录制
```

录制完成后，通过 USB MSC 访问 FAT 分区，复制 `capture.btsnoop` 到电脑。

### 4. Shell 查看

```
ble_sniff status         # 显示状态和统计
ble_sniff list [n]       # 显示最近 n 个数据包
ble_sniff list 5 1       # 显示最近 5 个数据包的详细信息
ble_sniff clear          # 清空缓存和计数
```

### 5. CDC Shell 切换

如需在抓包时禁用 CDC Shell 输出（避免干扰 pcapng 数据流），在 Shell 中执行：

```
cdc_shell 0              # 禁用 CDC Shell
cdc_shell 1              # 恢复 CDC Shell
```

## Wireshark 过滤器

捕获后在 Wireshark 中输入以下显示过滤器：

| 用途 | 过滤器 |
|------|--------|
| HCI 事件 | `hci_h4.type == 0x04` |
| 广播报告 | `btle.advertising_data` |
| 特定 MAC | `btle.advertising_address == xx:xx:xx:xx:xx:xx` |
| RSSI 范围 | `btle.rssi > -60` |

## Shell 命令一览

| 命令 | 说明 |
|------|------|
| `ble_sniff scan` | 开始扫描 |
| `ble_sniff stop` | 停止扫描 |
| `ble_sniff status` | 显示状态 |
| `ble_sniff list [n]` | 列出最近 n 个数据包 |
| `ble_sniff clear` | 清空缓存 |
| `ble_sniff record` | 开始 btsnoop 录制到 FAT |
| `ble_sniff stoprec` | 停止 btsnoop 录制 |
| `cdc_shell 0` | 禁用 CDC Shell |
| `cdc_shell 1` | 启用 CDC Shell |

## 工具文件清单

```
tools/ble_sniffer/
├── start_sniffer.cmd               # 一键启动 Wireshark（编辑 COM 口后双击）
├── esp_ble_sniffer_pipe.py         # 管道桥接到 Wireshark
├── esp_ble_sniffer_to_btsnoop.py   # CDC → btsnoop 文件转换
├── esp_ble_sniffer_bridge.py       # 命名管道桥接（备选）
├── README.md                       # 本文件
└── extcap/
    ├── esp_ble_sniffer_extcap.bat  # Wireshark extcap 包装器
    ├── esp_ble_sniffer_extcap.py   # Wireshark extcap 插件
    └── install_extcap.cmd          # extcap 部署工具（管理员运行）
```

## 架构说明

```
┌──────────────────────────────────────────────┐
│  ESP32-S3                                    │
│                                              │
│  BT Controller + Bluedroid                   │
│       │ GAP 扫描回调                          │
│       ▼                                       │
│  Send Advertising Report                      │
│       │                                       │
│       ├──→ pcapng → USB CDC → Wireshark      │
│       ├──→ btsnoop → FAT file → Ellisys      │
│       └──→ Ring Buffer → Shell List          │
│                                              │
└──────────────────┬───────────────────────────┘
                   │ USB-C
                   ▼
              PC 分析工具
```

## 注意事项

1. **CDC 端口共享**：pcapng 数据和 Shell 输出共用同一 CDC ACM 0，二进制 vs 文本互不干扰
2. **抓包范围**：基于 GAP 扫描，只能捕获广播信道（37/38/39）的广播包，不能抓连接后的数据信道
3. **退出手势**：在 sniffer 界面右滑即可退出，BT 资源自动释放
4. **FAT 录制路径**：`/spiflash/capture.btsnoop`（非 `S:/` 开头，那是 LVGL 的路径）
