# 多来源位置遥测

## ADDED Requirements

### Requirement: Wi-Fi 位置遥测
系统 SHALL 仅在 MQTT 已连接且本次 Wi-Fi 估算位置具有合法坐标、已知坐标系、有效 UTC 采集时间并在扫描开始后 60 秒内时，将结果发布到既有 user/update Topic；以 source、coordinateSystem、capturedAtUtc、accuracyM 和 accuracyStatus 明示来源、坐标系和精度状态，不推测缺失的精度。

#### Scenario: 精度未知的估算位置
- **WHEN** 用户明确请求上报且阿里云返回当前有效坐标和已知坐标系但不提供精度半径
- **THEN** 上报 source 为 wifi、accuracyM 为 null、accuracyStatus 为 unknown，并明确这是估算位置

#### Scenario: 未满足门槛
- **WHEN** Wi-Fi 定位不可用、坐标系未知、已过期或 MQTT 未连接
- **THEN** 系统拒绝发布该 Wi-Fi 位置且不使用旧坐标冒充
