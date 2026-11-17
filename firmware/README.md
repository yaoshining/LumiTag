# 固件

`LumiTagWiFi` 是当前原型的 Wi-Fi 与阿里云物联网平台连通性草图。它用于在 NB-IoT 卡恢复前完成局域网接入、云端接口和位置上报流程的开发。

## 烧录

在 Arduino IDE 1.8.x 中打开 `LumiTagWiFi/LumiTagWiFi.ino`，选择：

* 开发板：`TinkerNode NB-IoT (CH)`
* 端口：本开发板实际使用的 ESP32 USB 串口，例如 macOS 下的 `/dev/cu.usbserial-*`

烧录后打开串口监视器，设置为 `115200` 波特率和 `Newline` 行结束符。

## 串口命令

* `scan`：扫描附近的 Wi-Fi 网络。
* `ssid <网络名称>`：设置 SSID。
* `password <密码>`：设置密码。
* `connect`：发起连接，最长等待 20 秒。
* `status`：显示已连接网络、IP 地址和信号强度。
* `disconnect`：断开当前连接。
* `help`：显示命令说明。

## 阿里云 MQTT

已在阿里云物联网平台的华东 2（上海）公共实例创建 `LumiTag` 产品和 `LumiTag_ESP32_01` 原型设备。固件使用产品预置、具备发布权限的 `user/update` 自定义 Topic，无需额外创建 Topic。

从设备详情的“设备证书”获取 `ProductKey`、`DeviceName` 和 `DeviceSecret` 后，在串口监视器依次输入：

```text
cloud product <ProductKey>
cloud device LumiTag_ESP32_01
cloud secret <DeviceSecret>
cloud connect
cloud location 31.2304 121.4737
```

位置测试会发布到 `/<ProductKey>/<DeviceName>/user/update`，内容为包含 `latitude`、`longitude` 和 `uptimeMs` 的 JSON。设备密钥仅保存在运行内存，不会输出、写入闪存或提交到仓库。

当前 MQTT 接入使用本公共实例的 TLS `8883` 端口。固件内置阿里云 IoT 平台用于 MQTT `8883` 的 PCA 根证书，并在连接前校时后执行服务器证书与域名校验；不会跳过 TLS 校验。根证书来自阿里云的[根证书更新说明](https://help.aliyun.com/zh/iot/user-guide/announcement-on-the-update-of-the-alibaba-cloud-iot-platform-root-certificate)。

SSID、Wi-Fi 密码和设备证书均仅保存在运行内存，重启后清除；请不要将真实凭据写入代码或提交到仓库。
