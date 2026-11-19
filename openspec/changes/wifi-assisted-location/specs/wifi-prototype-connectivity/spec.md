## MODIFIED Requirements

### Requirement: 运行时 Wi-Fi 配置
系统 SHALL 通过串口接受 Wi-Fi 和云设备配置，仅在操作员明确执行 `config save` 后将其保存到设备非易失存储，并可在启动时读取并自动连接；凭据不得写入源码、文档、Git 或日志，不得通过串口输出。

#### Scenario: 使用当前配置连接 Wi-Fi
- **WHEN** 操作员依次设置 SSID、密码并发送 `connect`
- **THEN** 系统尝试连接并仅输出不包含网络标识和地址的状态

#### Scenario: 配置在重启后清除
- **WHEN** 设备没有保存过有效配置且重启
- **THEN** 未保存的运行时配置清除，系统提示操作员重新输入

#### Scenario: 保存当前配置
- **WHEN** 操作员提供完整配置并执行 `config save`
- **THEN** 系统将配置保存到设备 NVS，读回校验并仅输出成功或失败状态

#### Scenario: 重启后恢复
- **WHEN** 设备启动且存在完整有效的已保存配置
- **THEN** 系统在内存加载配置并自动尝试 Wi-Fi 与受认证 TLS MQTT 连接，不自动发布位置

#### Scenario: 没有保存的配置
- **WHEN** 设备启动但不存在有效已保存配置
- **THEN** 系统提示操作员输入，且不尝试使用残缺配置连接
