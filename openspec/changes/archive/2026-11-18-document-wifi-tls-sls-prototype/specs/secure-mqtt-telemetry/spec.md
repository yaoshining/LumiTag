# Spec Delta

## Purpose

定义 LumiTag 原型向阿里云物联网平台建立安全 MQTT 会话并上报位置遥测的外部行为，保证原型不会以明文凭据或跳过证书校验的方式接入云端。

## ADDED Requirements

### Requirement: 受验证的 MQTT TLS 会话
系统 SHALL 使用 MQTT TLS 端口 `8883`，校验服务器证书和域名，并使用 HMAC-SHA256 设备鉴权建立会话；系统不得使用跳过证书校验的连接方式。

#### Scenario: 连接成功
- **WHEN** Wi-Fi、产品标识、设备名称和设备密钥均已在运行内存中设置，且云端可达
- **THEN** `cloud connect` 输出 MQTT 已连接

#### Scenario: TLS 连接失败
- **WHEN** TLS 握手或服务器证书校验失败
- **THEN** 系统输出连接失败状态和 TLS 错误信息，且不输出设备密钥或鉴权签名

### Requirement: MQTT 鉴权时间同步
系统 SHALL 在 MQTT 鉴权前获取有效 Unix 时间；系统优先使用 NTP，在 NTP 不可用时使用阿里云 HTTP 时间接口回退校时。

#### Scenario: NTP 被网络阻断
- **WHEN** NTP 在限定时间内未能设置有效系统时间且 HTTPS/TLS 链路仍可用
- **THEN** 系统尝试 HTTP 时间回退并在成功后继续 MQTT 鉴权

#### Scenario: 所有时间源不可用
- **WHEN** NTP 和 HTTP 时间回退均失败
- **THEN** 系统拒绝发起 MQTT 鉴权并报告时间同步失败

### Requirement: 位置遥测上报
系统 SHALL 在已连接的 MQTT 会话中，将纬度、经度和运行时长 JSON 发布到 `/<ProductKey>/<DeviceName>/user/update`。

#### Scenario: 发布测试位置
- **WHEN** 操作员在 MQTT 已连接时发送 `cloud location <latitude> <longitude>`
- **THEN** 系统向设备的 `user/update` Topic 发布 JSON 并报告发布成功

#### Scenario: 未连接时发布
- **WHEN** 操作员在 MQTT 未连接时发送位置上报命令
- **THEN** 系统拒绝发布并提示先连接阿里云 MQTT
