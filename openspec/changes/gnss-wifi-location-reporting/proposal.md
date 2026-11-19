# Proposal

## Why
现有位置上报仅使用手工测试坐标，无法证明定位数据来自 GNSS 或满足质量要求。

## What Changes
- 增加 BC20 有限时长定位与质量门槛，关闭 GNSS 并拒绝过期定位。
- 增加串口操作和通过既有 TLS MQTT Topic 上报 GNSS JSON，移除手工坐标发布入口。
- 分层记录编译、烧录、实机及云端验收；不建立位置存储资源。

## Capabilities
### New Capabilities
- `gnss-location`: BC20 定位、质量控制与生命周期。

### Modified Capabilities
- `secure-mqtt-telemetry`: 增加 GNSS 位置上报。

## Impact
影响固件、串口命令、项目文档及 OpenSpec；依赖本机官方 DFRobot_NBIoT 库，不改变云资源。
