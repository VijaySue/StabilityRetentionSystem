#!/bin/bash

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "请使用root权限运行此脚本"
    exit 1
fi

# 停止服务
systemctl stop stability-server

# 禁用服务
systemctl disable stability-server

# 删除服务文件
rm -f /etc/systemd/system/stability-server.service

# 重新加载systemd配置
systemctl daemon-reload

echo "系统服务已卸载。" 