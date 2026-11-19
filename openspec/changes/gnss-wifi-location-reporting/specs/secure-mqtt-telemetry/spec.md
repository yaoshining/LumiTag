# 安全遥测增量

## ADDED Requirements

### Requirement: GNSS 位置遥测
系统 SHALL 仅在 MQTT 已连接且存在符合 GNSS 定位质量和时效要求的数据时，将 GNSS JSON 发布到既有设备 `user/update` Topic。

#### Scenario: 合格定位上报
- **WHEN** 用户发送 `cloud location gnss` 且当前定位合格并且 MQTT 已连接
- **THEN** JSON 包含 latitude、longitude、altitudeM、speedKmh、headingDeg、capturedAtUtc、fixMode、satellitesUsed、hdop 和 source 为 gnss

#### Scenario: 缺少连接或有效定位
- **WHEN** MQTT 未连接、定位失败或定位已过期
- **THEN** 系统明确报告原因且不发布位置

## MODIFIED Requirements

### Requirement: 位置遥测上报
系统 SHALL 只在已连接的 MQTT 会话中，将符合对应来源质量和时效要求的位置 JSON 发布到 `/<ProductKey>/<DeviceName>/user/update`，不得发布手动输入的坐标；GNSS 质量门槛保持不变，Wi-Fi 估算遵循 Wi-Fi 遥测规格并明确标记未知精度。

#### Scenario: 发布合格位置
- **WHEN** 操作员在 MQTT 已连接且具有当前合格定位时发送 `cloud location gnss`
- **THEN** 系统向设备的 `user/update` Topic 发布 GNSS JSON 并报告发布成功

#### Scenario: 未连接时发布
- **WHEN** 操作员在 MQTT 未连接时发送位置上报命令
- **THEN** 系统拒绝发布并提示先连接阿里云 MQTT

#### Scenario: 发布测试位置
- **WHEN** 操作员输入手动坐标上报命令
- **THEN** 系统拒绝该命令且不发布位置
