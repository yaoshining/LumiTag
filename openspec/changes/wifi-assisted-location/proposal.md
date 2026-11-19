# Proposal

## Why
室内 GNSS 经常不可用，需要评估覆盖家、学校等地点的 Wi-Fi 辅助定位，且不得把未知或过期位置伪装成当前定位。

## What Changes
- 增加仅在内存中的 Wi-Fi 指纹采集和不泄露标识的诊断。
- 接入已获用户授权的阿里云 TLS 定位服务，将当前有效的 Wi-Fi 估算结果以带来源、坐标系和明确未知精度的形式上报。
- 按来源和时效管理 GNSS 与 Wi-Fi 结果，禁止 IP 假定位和未经确认的云资源或计费变更。
- 用户明确要求减少重复输入后，允许手动一次保存设备 Wi-Fi 与云设备配置到设备 NVS，重启自动加载；设备未确认硬件加密，必须明确物理读取风险。

## Capabilities
### New Capabilities
- `wifi-assisted-location`: Wi-Fi 指纹采集、服务解析与结果质量。

### Modified Capabilities
- `secure-mqtt-telemetry`: 待 GNSS 遥测变更完成后扩展支持标记来源的 Wi-Fi 位置。

## Impact
涉及 ESP32 扫描、定位服务选择、固件和遥测格式；不默认开通定位服务或提交 Wi-Fi 指纹。
