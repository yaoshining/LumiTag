# 阿里云 Wi-Fi 定位一次性验证

此示例按阿里云 [HTTPS 设备认证](https://help.aliyun.com/zh/iot/user-guide/establish-connections-over-https) 和 [HTTP 定位](https://help.aliyun.com/zh/iot/user-guide/http-geolocation) 接口，在华东 2（上海）公共实例发送一次请求。成功时平台可能写入设备物模型的位置属性；定位服务已停止新开通，只有此前开通的用户可继续使用。不会创建或修改云资源，不建立原始 payload 入库链路。

在本机终端运行：

```sh
python3 tools/diagnostics/aliyun_wifi_location/probe.py
```

按提示在终端内输入设备 ProductKey、DeviceName、DeviceSecret，以及 2～30 个不同的**固定热点**，每行格式为 `BSSID,负值信号强度,SSID`；最后输入空行。信息均不回显，不要通过聊天、命令行参数、文件或日志提供。程序不保存或打印凭据、热点信息和精确坐标；必须具备真实扫描结果，不能用示例值代替。不要输入手机热点。阿里云要求的其他供应商特定参数仍可能影响请求。

工具使用系统 CA 校验 HTTPS、不使用代理、不跟随重定向，并仅输出认证和定位的状态码。成功只说明服务返回了合法坐标，未验证位置准确性；失败也不能单凭一次错误码判断服务是否已开通。官方前置条件是产品有 `GeoLocation` 物模型属性；当前 LumiTag 已发布物模型未见该属性，因此本测试即使失败也可能无法区分权限问题与配置问题。配置或发布物模型属于云端变更，本工具不会执行。

如果当前没有真实热点信息，可先通过设备或系统的 Wi-Fi 扫描功能在本机查看；现有 `wifi location scan` 为保护隐私只显示数量，不能直接为此工具提供输入。

## 本机私密距离比较

开发板使用 `cloud location wifi private` 可将当前返回坐标显示在本机串口。用户可另开终端运行 `python3 tools/diagnostics/aliyun_wifi_location/compare_private.py`，依次在不回显的提示中输入设备与参考点坐标；只显示两点近似直线距离，不写文件或发送网络。请关闭终端/串口日志，不将坐标输入命令行参数或聊天。两组坐标必须是同一坐标系：阿里云代码 2 是 GCJ-02，不能直接与 WGS-84 坐标比较。参考定位本身可能有误差，距离不等于服务给出的精度半径。
