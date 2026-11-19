#!/usr/bin/env python3
"""本机比较同一坐标系的两点；输入不回显，不写文件或发送网络。"""
import getpass
import math
import sys


def read_coordinate(prompt, lower, upper):
    value = float(getpass.getpass(prompt + "（不回显）: ").strip())
    if not math.isfinite(value) or not lower <= value <= upper:
        raise ValueError
    return value


try:
    print("两组坐标必须使用同一坐标系；设备代码 2 表示 GCJ-02。")
    print("参考点须为设备当前所在位置；参考定位自身误差会影响结果。")
    lat1 = read_coordinate("设备纬度", -90, 90)
    lon1 = read_coordinate("设备经度", -180, 180)
    lat2 = read_coordinate("参考纬度", -90, 90)
    lon2 = read_coordinate("参考经度", -180, 180)
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    distance = 6371000 * 2 * math.asin(math.sqrt(min(1, max(0, a))))
    print(f"两点直线距离约 {distance:.0f} 米；该数值不是服务返回的精度半径。")
except (ValueError, EOFError, KeyboardInterrupt):
    print("\n输入无效或已取消。", file=sys.stderr)
    sys.exit(1)
