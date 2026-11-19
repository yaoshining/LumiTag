# Spec Delta

## Purpose

定义 LumiTag 原型在 NB-IoT 可用前通过 ESP32 Wi-Fi 接入网络的交互、诊断与凭据处理边界，使现场调试具有可重复的可观测结果。

## ADDED Requirements

### Requirement: 运行时 Wi-Fi 配置
系统 SHALL 通过串口接受 SSID 和密码，并在显式连接命令后尝试接入 2.4 GHz Wi-Fi；凭据不得写入 Flash 或串口输出。

#### Scenario: 使用当前配置连接 Wi-Fi
- **WHEN** 操作员依次设置 SSID、密码并发送 `connect`
- **THEN** 系统在连接成功时输出已连接网络、IP 地址和信号强度

#### Scenario: 配置在重启后清除
- **WHEN** 设备复位后收到 `connect`
- **THEN** 系统要求先设置 SSID，且不使用复位前的 Wi-Fi 凭据

### Requirement: Wi-Fi 状态与链路诊断
系统 SHALL 提供 Wi-Fi 状态查询和云端链路探测；探测结果必须分别反映 DNS 解析、TCP 8883 连接和 TLS 服务器证书校验。

#### Scenario: 查询已连接状态
- **WHEN** 操作员在 Wi-Fi 已连接时发送 `status`
- **THEN** 系统输出当前 SSID、IP 地址和 RSSI

#### Scenario: 探测阿里云 MQTT 链路
- **WHEN** 操作员在 Wi-Fi 已连接时发送 `cloud probe`
- **THEN** 系统按 DNS、TCP 和 TLS 阶段输出成功或失败结果，且不输出设备密钥
