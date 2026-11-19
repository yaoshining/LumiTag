#!/usr/bin/env python3
"""交互式验证阿里云设备 Wi-Fi 定位；凭据和热点信息仅保存在进程内。"""

import getpass
import hashlib
import hmac
import json
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

ENDPOINT = "https://iot-as-http.cn-shanghai.aliyuncs.com"
MAC = re.compile(r"^(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$")
NAME = re.compile(r"^[A-Za-z0-9._:-]{1,64}$")


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def post(path, payload, content_type, token=None):
    headers = {"Content-Type": content_type}
    if token:
        headers["password"] = token
    req = urllib.request.Request(ENDPOINT + path, json.dumps(payload).encode(), headers, method="POST")
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    try:
        with opener.open(req, timeout=20) as response:
            return response.status, json.loads(response.read(16384))
    except urllib.error.HTTPError as exc:
        try:
            return exc.code, json.loads(exc.read(16384))
        except (ValueError, UnicodeError):
            return exc.code, {}


def auth_payload(product, device, secret):
    fields = {"clientId": "lumitag-location-probe", "deviceName": device,
              "productKey": product, "timestamp": str(int(time.time() * 1000))}
    text = "".join(key + fields[key] for key in sorted(fields))
    sign = hmac.new(secret.encode(), text.encode(), hashlib.sha1).hexdigest()
    return {**fields, "signmethod": "hmacsha1", "sign": sign, "version": "default"}


def read_access_points():
    print("输入 2～30 个固定 Wi-Fi 热点，每行格式：BSSID,信号强度,SSID；空行结束。")
    print("输入不回显；请勿输入移动热点。")
    points = []
    while True:
        row = getpass.getpass(f"热点 {len(points) + 1}: ").strip()
        if not row:
            break
        parts = row.split(",", 2)
        if len(parts) != 3 or not MAC.fullmatch(parts[0]) or not re.fullmatch(r"-\d{1,3}", parts[1]) or not parts[2] or "|" in parts[2]:
            raise ValueError("热点格式无效")
        if int(parts[0][:2], 16) & 1 or not -127 <= int(parts[1]) <= -1:
            raise ValueError("热点地址或信号强度无效")
        if any(p.split(",", 1)[0].lower() == parts[0].lower() for p in points):
            raise ValueError("热点地址重复")
        points.append(row)
        if len(points) > 30:
            raise ValueError("热点超过 30 个")
    if len(points) < 2:
        raise ValueError("至少需要两个不同的固定热点")
    return points


def run():
    print("一次性 Wi-Fi 定位测试：会向阿里云上传热点信息；成功时平台可能写入设备位置属性。")
    print("请勿录屏或分享热点、密钥、Token 和精确位置。")
    product = getpass.getpass("ProductKey（不回显）: ").strip()
    device = getpass.getpass("DeviceName（不回显）: ").strip()
    secret = getpass.getpass("DeviceSecret（不回显）: ").strip()
    if not NAME.fullmatch(product) or not NAME.fullmatch(device) or not secret:
        raise ValueError("设备身份格式无效")
    points = read_access_points()
    status, result = post("/auth", auth_payload(product, device, secret), "application/json")
    token = result.get("info", {}).get("token") if isinstance(result.get("info"), dict) else None
    if status != 200 or result.get("code") != 0 or not isinstance(token, str) or not token:
        code = result.get("code")
        print(f"设备认证失败：HTTP {status}，code={code if type(code) is int else '未知'}。")
        return 1
    print("设备认证成功；正在进行一次 Wi-Fi 定位请求。")
    payload = {"id": "1", "version": "1.0", "params": {"identifier": "Location.Position", "serviceParams": {"type": "wifi", "macs": "|".join(points)}}, "method": "_thing.service.post"}
    path = "/topic/sys/{}/{}/_thing/service/post".format(urllib.parse.quote(product, safe=""), urllib.parse.quote(device, safe=""))
    status, result = post(path, payload, "application/octet-stream", token)
    code = result.get("code")
    if status == 200 and code == 0:
        info = result.get("info")
        if not isinstance(info, dict) or not all(isinstance(info.get(k), (int, float)) and not isinstance(info.get(k), bool) for k in ("latitude", "longitude")) or not (-90 <= info["latitude"] <= 90 and -180 <= info["longitude"] <= 180):
            print("服务返回成功但未提供合法坐标，不能判定定位成功。")
            return 1
        system = info.get("CoordinateSystem")
        print(f"服务返回有效坐标；坐标系代码：{system if type(system) is int else '未提供'}。为保护隐私不显示坐标。")
        print("结果不含可靠精度半径；未验证坐标实际准确性。")
        return 0
    print(f"定位请求未成功：HTTP {status}，code={code if type(code) is int else '未知'}。")
    print("6731 可能是服务不可访问；6732 参数有误；6733 热点未匹配。其他错误需结合官方文档判断。")
    print("单次失败不能单独证明服务未开通；请同时检查物模型、设备权限和热点数据。")
    return 1


if __name__ == "__main__":
    try:
        sys.exit(run())
    except (ValueError, OSError, json.JSONDecodeError):
        print("输入无效、网络或证书校验失败；未输出敏感内容。", file=sys.stderr)
        sys.exit(1)
    except (KeyboardInterrupt, EOFError):
        print("\n已取消。", file=sys.stderr)
        sys.exit(1)
