# Tasks

## 1. 规格与操作文档

- [x] 1.1 编写 Wi-Fi 原型连接规格，并以 `status` 与 `cloud probe` 的串口输出定义可验证行为
- [x] 1.2 编写 TLS MQTT 遥测规格，并以成功的 TLS 会话和 `user/update` 发布定义验收
- [x] 1.3 编写 SLS 运行日志转储规格，并以 `uplink`、Topic 和 `200` 查询结果定义验收
- [x] 1.4 更新固件操作说明，核对命令、凭据边界和时间回退说明与实现一致

## 2. 现场与云端验收记录

- [x] 2.1 验证 DFR0530 串口、下载器和基础按键/RGB 路径；以 ESP32 芯片探测与物理观察为证据
- [x] 2.2 验证设备端 DNS、TCP `8883` 与 TLS 证书校验；以 `cloud probe` 三阶段成功输出为证据
- [x] 2.3 验证 MQTT TLS 上线和位置消息发布；以 IoT 平台 `online`、TLS 1.2 与上行成功状态为证据
- [x] 2.4 验证 SLS 运行日志落库；以 `iot-logs` 中的 `uplink`、`user/update` 与 `200` 记录为证据

## 3. OpenSpec 归档

- [x] 3.1 校验变更包，确认 proposal、design、specs 和 tasks 完整且无未完成任务
- [x] 3.2 归档变更并将三项已验证能力写入主 OpenSpec 规格目录
