# Proposal

## Why

LumiTag 原型已完成从 ESP32 Wi-Fi 到阿里云物联网平台和 SLS 的真实链路验证，但仓库此前只有愿景性说明，缺少可验收的行为规格、部署边界和现场证据。现在补齐规格，避免将原型能力与尚未实施的 NB-IoT、GNSS、墨水屏和低功耗能力混为一谈。

## What Changes

- 记录运行时 Wi-Fi 配置、串口交互和凭据不落盘的行为要求。
- 记录基于服务器证书校验的 MQTT TLS 连接、时间同步回退和位置遥测上报要求。
- 记录阿里云 IoT 平台运行日志转储到 SLS 的配置结果和查询边界。
- 将现场硬件、网络和云端验收证据归档到设计与任务文档。

## Capabilities

### New Capabilities

- `wifi-prototype-connectivity`: 原型设备通过串口进行临时 Wi-Fi 配置并暴露可诊断的连接状态。
- `secure-mqtt-telemetry`: 原型设备通过经证书验证的 TLS MQTT 连接阿里云并发布位置遥测。
- `cloud-runtime-log-storage`: 阿里云 IoT 运行日志转储到 SLS，支持检索已接收的上行消息。

### Modified Capabilities

- 无。

## Impact

- 固件：`firmware/LumiTagWiFi/LumiTagWiFi.ino`。
- 操作文档：`firmware/README.md`、根目录 README。
- 云端：阿里云物联网平台公共实例、SLS 项目和 Logstore。
- 不引入新的仓库依赖；设备、Wi-Fi 和阿里云证书均继续只保存在运行内存。
