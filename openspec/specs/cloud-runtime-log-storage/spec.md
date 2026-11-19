# cloud-runtime-log-storage Specification

## Purpose
定义 LumiTag 设备在阿里云物联网平台产生的云端运行日志如何转储到 SLS，以便在设备发布后保留可检索的接入和上行消息证据。

## Requirements

### Requirement: 云端运行日志转储
阿里云 IoT 平台 SHALL 为 LumiTag 产品启用云端运行日志转储，并将日志写入同地域自动创建的 SLS Project 与 Logstore。

#### Scenario: 转储服务已启用
- **WHEN** 操作员查看物联网平台的“云端运行日志转储”
- **THEN** 控制台显示转储已启用，且存在对应的 SLS 存储位置

### Requirement: 上行消息可检索
系统 SHALL 在 SLS 中保留设备成功发布到 `user/update` Topic 的运行日志，记录至少包含设备名称、操作 Topic、上行业务类型和成功状态。

#### Scenario: 查询已发布的遥测消息
- **WHEN** 设备成功发布一条 `user/update` 消息后，操作员查询对应 SLS Logstore
- **THEN** 查询结果包含 `uplink` 业务类型、该设备名称、Topic 和成功状态 `200`

### Requirement: 日志存储费用边界
文档 SHALL 将 SLS 描述为按量计费服务，并说明免费额度为账户级共享配额；不得将小流量免费额度表述为无限期免费承诺。

#### Scenario: 评估 SLS 成本
- **WHEN** 操作员依据项目文档判断是否继续使用 SLS
- **THEN** 文档要求以当前 SLS 控制台账单和官方计费页面为准，并提示免费额度可能被其他服务共享
