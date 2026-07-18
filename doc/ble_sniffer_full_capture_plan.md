# BLE Sniffer — Phase A/B/C 完整实施记录

> 最后更新: 2026-07-18

## 当前状态

| 阶段 | 状态 | 文件 |
|------|------|------|
| **A**: VHCI 基础架构 | ✅ **已实现** | `ble_sniffer_hci.c/h`, `ble_sniffer.c` 重构 |
| **B**: 测试模式 + VS 命令 | ✅ **已实现** | `ble_sniffer_vs.c/h` |
| **C**: LL PDU 解析 + CSA #1 | ✅ **已实现** | `ble_sniffer_ll.c/h` |
| **UI**: btsnoop 录制按钮 | ✅ **已实现** | `ble_sniffer.c` 中 `btn_record` |

## 文件清单

```
components/ble_sniffer/
├── ble_sniffer.c               # ✅ RTAM 入口, VHCI 驱动 (from GAP)
├── ble_sniffer_hci.c           # ✅ NEW - VHCI 回调 + HCI 命令 + 事件处理任务
├── ble_sniffer_hci.h           # ✅ NEW
├── ble_sniffer_vs.c            # ✅ NEW - 内部测试命令 (Phase B)
├── ble_sniffer_vs.h            # ✅ NEW
├── ble_sniffer_ll.c            # ✅ NEW - CONNECT_REQ 解析 + CSA #1 (Phase C)
├── ble_sniffer_ll.h            # ✅ NEW
├── ble_sniffer_pcap.c          # ✅ 未改动
├── ble_sniffer_pcap.h          # ✅ 未改动
├── ble_sniffer_btsnoop.c       # ✅ 未改动
├── ble_sniffer_btsnoop.h       # ✅ 未改动
├── icon_app_ble_sniffer.c      # ✅ 未改动
├── CMakeLists.txt              # ✅ 更新 - 添加新源文件
```

## 架构

```
app_main → start_task → rtamInit
                           ↓
                    ble_sniff (RTAM app)
                           │
              ┌────────────┴────────────┐
              │ start()                 │ resume()
              ▼                         ▼
    bt_manager_init_sniffer()      LVGL UI screen
    hci_init(packet_cb)             ├─ status_label
    hci_init_scan()                 ├─ count_label
              │                     ├─ btn_toggle [Scan|Stop]
              │                     └─ btn_record [Record|Stop Rec]
              ▼
    BT Controller (no Bluedroid)
              │
         VHCI 回调
              │
         hci_evt_queue
              │
         hci_proc_task (core 0)
              │
         process_hci_event()
              │
         on_hci_packet() callback
              │
         ┌────┼─────────┬──────────┐
         │    │         │          │
      pcapng btsnoop  ring      USB CDC
      → CDC  → FAT    buffer    → Wireshark
```

## 关键改动

### bt_manager
- 新增 `bt_manager_is_sniffer_mode()` — 查询是否 sniffer 模式
- `bt_manager_init_sniffer()` — 初始化 Controller-Only 模式
- RTAM `conflicted` 机制确保 sniffer 启动时 `ble_remote` 停止

### ble_sniffer.c
- 移除 `esp_gap_ble_api.h` 依赖（不再使用 GAP API）
- 新增 `esp_bt.h`, `ble_sniffer_hci.h`, `ble_sniffer_vs.h`, `ble_sniffer_ll.h`
- 入口改用 `bt_manager_init_sniffer(NULL)` + `hci_init()`
- 扫描控制改为 `hci_start_scan()` / `hci_stop_scan()`
- `packet_count` 从 `hci_get_packet_count()` 获取
- 新增 `on_hci_packet()` 回调处理 VHCI 事件

### ble_sniffer_hci.c (新增)
- VHCI 回调注册 (`esp_vhci_host_register_callback`)
- HCI 命令构建：Reset / SetEventMask / LE SetScanParams / LE SetScanEnable
- HCI 事件队列 + 处理任务
- LE Advertising Report 解析 → 用户回调

### ble_sniffer_vs.c (新增, Phase B)
- `esp_ble_internalTestFeaturesEnable(true)` 包装
- VS 命令发送：`hci_send_cmd` 通过 VHCI 发送
- `vs_set_scan_aa()`, `vs_set_scan_channel()`, `vs_set_expected_peer()`

### ble_sniffer_ll.c (新增, Phase C)
- CONNECT_REQ PDU 解析：提取 AA, CRCInit, Interval, Latency, Timeout, ChM, Hop
- CSA #1 跳频算法实现
- 连接事件时间预测

## UI

```
┌────────────────────┐
│ Sniffing + REC     │  ← status_label (定时器更新)
│                    │
│ Packets: 1234      │  ← count_label
│                    │
│ [Scan ] [Record]   │  ← btn_toggle + btn_record
└────────────────────┘
```

底部两按钮：左=扫描启停，右=btsnoop 录制启停（FAT）。

## Shell 命令

| 命令 | 说明 |
|------|------|
| `ble_sniff scan` | 开始扫描 |
| `ble_sniff stop` | 停止扫描 |
| `ble_sniff status` | 状态 + 包计数 |
| `ble_sniff list [n]` | 最近 n 包 |
| `ble_sniff clear` | 清空 |
| `ble_sniff record` | 开始 btsnoop 混录到 FAT |
| `ble_sniff stoprec` | 停止混录 |
| `ble_sniff vsenable` | 启用 VS test mode + scan forever |
| `ble_sniff vsdisable` | 禁用 VS test mode |
| `ble_sniff scanchan <37|38|39>` | 锁定扫描指定广播信道 |
| `ble_sniff counters` | 查询 VS test mode 内部计数器 |

## 核心限制

烧录验证发现：**VS test mode + `SET_SCAN_AA` 不改变 VHCI 上报格式**。

控制器 VHCI 回调始终只接收标准 HCI 事件（LE Advertising Report）。CONNECT_REQ 被控制器 LL 层处理，不转发到 Host。

要捕获连接数据，需要改用 **Central 模式**（ESP32 主动连接目标设备）。

## 已知限制

- **无法抓 CONNECT_REQ 和数据信道包** — 控制器固件不通过 VHCI 上报原始 LL PDU
- 退出 sniffer 时 BT 完全释放，切换耗时 ~1-2 秒
- 长远方案：使用第二块芯片作为专用嗅探器（nRF52840 / 第二 ESP32）
