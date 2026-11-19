# Design

## Context

见 `proposal.md` 的动机说明。当前原型使用 DFR0530 TinkerNode NB-IoT 开发板的 ESP32 部分，经 2.4 GHz Wi-Fi 验证云端路径；NB-IoT 卡、GNSS、墨水屏、深度睡眠和量产安全方案尚未验收。

已确认的现场与云端事实：

- 开发板为 ESP32D0WDQ5（16 MB Flash）；稳定串口为 macOS `/dev/cu.usbserial-1130`、`115200`、`Newline`。
- `SET` 是应用侧 GPIO13 按键，不是下载引脚；下载器异常时须重新插拔 USB。低速分段写入可降低此板的烧录失败率。
- 短按 `SET` 可切换示例模式；蓝色 LED 为 GPIO2，WS2812 RGB 数据引脚为 GPIO12。该结论只说明原型板功能路径，不能替代最终硬件验证。
- 云端 MQTT 端点的 DNS、TCP `8883`、TLS 握手和服务器证书校验均在设备上成功；系统通过 TLS 1.2 设备会话上线，并成功发布 `user/update` 上行消息。
- 当前 Wi-Fi 下 UDP/NTP 不可用；阿里云 HTTP 时间回退成功后用于生成 MQTT 鉴权时间戳。
- 已启用云端运行日志转储，SLS `iot-logs` 中已查到本设备 `uplink`、`200` 的 `user/update` 发布记录。

## Goals / Non-Goals

**Goals:**

- 用可测试的规格描述当前原型的实际网络能力和凭据边界。
- 保留每项能力的现场验收证据，区分“已观察到”与“计划中”。
- 让后续开发者可以按串口命令重现 Wi-Fi、TLS MQTT 和日志查询。

**Non-Goals:**

- 不将 Wi-Fi 原型替代为 NB-IoT 生产实现。
- 不将 HTTP 时间接口当作受认证的安全时间源。
- 不实现 GNSS 获取、墨水屏 UI、深度睡眠、电池寿命或地理围栏。
- 不把 DeviceSecret、Wi-Fi 密码、SIM 标识或私网 IP 写入仓库。

## Decisions

### 运行时串口配置，不保存凭据

SSID、Wi-Fi 密码和设备密钥仅驻留 RAM。这样在原型调试中可直接输入凭据，同时避免 Flash、代码库和串口日志泄露。替代方案是写入 NVS；因其扩大了本地凭据暴露面且不适合当前验证范围，未采用。

### MQTT 使用 TLS 8883 和 CA 校验

固件嵌入阿里云 IoT MQTT PCA 根证书，并使用 `WiFiClientSecure` 的 CA 校验和主机名校验。替代方案 `setInsecure()` 会失去服务器身份验证，已明确排除。设备密钥只用于计算 HMAC-SHA256 鉴权签名，不写入或打印。

### NTP 首选，HTTP 仅作为时间回退

NTP 是标准时间来源；在当前 Wi-Fi 阻断 UDP/NTP 时，固件从阿里云 HTTP 时间接口读取 Unix 毫秒，以生成 MQTT 鉴权时间戳。HTTP 时间只影响连接新鲜度，不能取代最终 MQTT TLS 的证书校验，也不携带设备密钥。替代方案是人工输入时间，操作错误率更高，未采用。

### 通过 IoT 云端运行日志转储进入 SLS

使用 IoT 平台原生“云端运行日志转储”，让平台自动创建同地域 SLS 存储位置。替代方案是用云产品流转直接转发；当前实例的可选目的地不包含 SLS，且原生转储更适合接入运行日志保留。

## Risks / Trade-offs

- [HTTP 时间响应可被篡改] -> 回退仅设置鉴权时间戳；MQTT 仍必须完成 CA 校验 TLS 和 HMAC 设备鉴权。量产前应改为可认证时间源或具备可信 RTC。
- [Wi-Fi 断网、DNS 或 TCP 失败] -> `cloud probe` 分段输出 DNS、TCP 和 TLS 结果，避免把网络失败误判为设备密钥错误。
- [SLS 费用超过免费额度] -> 采用小流量原型，按 SLS 账单和官方计费页复核；免费额度为账户级共享，不承诺无限免费。
- [串口下载器状态不稳定] -> 保持 `115200`，必要时 USB 重新插拔，再以分段写入和 Hash 校验恢复。
- [日志包含敏感运行信息] -> 不在仓库记录日志原文、设备密钥、SIM 标识或公网/私网 IP；生产环境需设置访问控制与保留策略。

## Migration Plan

1. 使用 `firmware/README.md` 的串口流程配置临时凭据并验证 `cloud probe`。
2. 使用 `cloud connect` 建立 TLS MQTT，再以 `cloud location` 发送测试遥测。
3. 在 IoT 日志服务和 SLS Logstore 中确认 `uplink`、Topic 和 `200` 状态。
4. 若需要回退，刷回已知可工作的上一份 `LumiTagWiFi` 构建产物；该回退不会恢复 RAM 凭据，操作员必须重新输入。
